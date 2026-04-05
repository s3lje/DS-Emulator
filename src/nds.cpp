#include "nds.h"
#include "io/ioregs.h"
#include <iostream>
#include <fstream>
#include <cstring>


bool NDS::loadROM(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if (!f){
        std::cerr << "Could not open ROM: " << path << std::endl;
        return false;
    }

    f.seekg(0, std::ios::end);
    bus.rom.resize(f.tellg());
    f.seekg(0);
    f.read(reinterpret_cast<char*>(bus.rom.data()), bus.rom.size());

    header = *reinterpret_cast<NDSHeader*>(bus.rom.data());

    std::cout << "Game: " << header.gameTitle << std::endl;
    std::cout << "ARM9: " << header.arm9Size << "bytes @ 0x"
              << std::hex << header.arm9RamAddress << std::endl;
    std::cout << "ARM7: " << header.arm7Size << "bytes @ 0x"
              << std::hex << header.arm7RamAddress << std::endl;

    // Copying ARM9 code from ROM to correct RAM address
    std::memcpy(
        &bus.mainRAM[header.arm9RamAddress - 0x02000000],
        bus.rom.data() + header.arm9RomOffset,
        header.arm9Size
    );

    arm9.setPC(header.arm9EntryAddress);
    arm7.setPC(header.arm7EntryAddress);

    return true;
}

void NDS::run(){
    // Placeholder for the scanline loop
    while (true){
        runFrame(); 
    }
}

void NDS::runFrame(){
    for (int line = 0; line < 263; line++){
        bus.vcount = line;
        runScanline(line);

        if (line == 192)
            fireVBlank();
    }
}

void NDS::runScanline(int line){
    int cycles9 = 0, cycles7 = 0;

    while (cycles9 < CyclesPerScanline9){
        arm9.step();
        arm9.checkInterrupts();
        cycles9++;

        // ARM7 runs every other cycle
        if (cycles9 % 2 == 0){
            arm7.step();
            arm7.checkInterrupts();
            cycles7++;
        }
    }

    // tick the timers 
    bus.timers9.tick(CyclesPerScanline9, bus.irq9);
    bus.timers7.tick(CyclesPerScanline7, bus.irq7);

    if (line < 192)
        fireHBlank(); 
}

void NDS::fireVBlank(){
    bus.irq9.raise(IRQ::VBlank);
    bus.irq7.raise(IRQ::VBlank); 
}

void NDS::fireHBlank(){
    bus.irq9.raise(IRQ::HBlank);
    bus.irq7.raise(IRQ::HBlank); 
}
