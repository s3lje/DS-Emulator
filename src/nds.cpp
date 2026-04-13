#include "nds.h"
#include "gpu/gpu2d.h"
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
    

    // Print first 32 bytes raw so we can see what we got
    std::cout << "First 32 ROM bytes: ";
    for (int i = 0; i < 32; i++)
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)bus.rom[i] << " ";
    std::cout << "\n";

    header = *reinterpret_cast<NDSHeader*>(bus.rom.data());

    std::cout << "Game: " << header.gameTitle << std::endl;
    std::cout << "ARM9: " << header.arm9Size << "bytes @ 0x"
              << std::hex << header.arm9RamAddress << std::endl;
    std::cout << "ARM7: " << header.arm7Size << "bytes @ 0x"
              << std::hex << header.arm7RamAddress << std::endl;

    std::cout << "Title:      " << std::string(header.gameTitle, 12) << "\n";
    std::cout << "ARM9 offset: 0x" << std::hex << header.arm9RomOffset << "\n";
    std::cout << "ARM9 entry:  0x" << std::hex << header.arm9EntryAddress << "\n";
    std::cout << "ARM9 ram:    0x" << std::hex << header.arm9RamAddress << "\n";
    std::cout << "ARM9 size:   0x" << std::hex << header.arm9Size << "\n";
    std::cout << "ARM7 offset: 0x" << std::hex << header.arm7RomOffset << "\n";
    std::cout << "ARM7 entry:  0x" << std::hex << header.arm7EntryAddress << "\n";
    std::cout << "ARM7 ram:    0x" << std::hex << header.arm7RamAddress << "\n";
    std::cout << "ARM7 size:   0x" << std::hex << header.arm7Size << "\n";
    // Copying ARM9 code from ROM to correct RAM address
    

    // Validate before copying
    auto arm9Off = header.arm9RamAddress - 0x02000000;
    if (arm9Off + header.arm9Size > bus.mainRAM.size()) {
        std::cerr << "ARM9 load out of bounds! offset=0x" << std::hex
                  << arm9Off << " size=0x" << header.arm9Size << "\n";
        return false;
    }
    if (header.arm9RomOffset + header.arm9Size > bus.rom.size()) {
        std::cerr << "ARM9 ROM offset out of bounds!\n";
        return false;
    }

    std::memcpy(
        &bus.mainRAM[arm9Off],
        bus.rom.data() + header.arm9RomOffset,
        header.arm9Size
    );

    arm9.setPC(header.arm9EntryAddress);
    arm7.setPC(header.arm7EntryAddress);

    std::cout << "ARM9 entry: 0x" << std::hex << header.arm9EntryAddress << "\n";
    std::cout << "ARM7 entry: 0x" << std::hex << header.arm7EntryAddress << "\n";
    std::cout << "ARM9 initial PC: 0x" << arm9.r[15] << "\n";
    std::cout << "ARM7 initial PC: 0x" << arm7.r[15] << "\n";

    return true;
}

void NDS::run(){
    if (!frontend.init()) return;

    while (running){
        running = frontend.pollEvents(bus.keyinput);
        runFrame(); 
    }
    
    frontend.shutdown();
}

void NDS::runFrame(){
    for (int line = 0; line < 263; line++){
        bus.vcount = line;

        if (line % 10 == 0)
            running = frontend.pollEvents(bus.keyinput);

        if (!running) return;


        runScanline(line);
        if (line == 192)
            fireVBlank();
    }
}

void NDS::runScanline(int line){
    int cycles9 = 0, cycles7 = 0;

    while (cycles9 < CyclesPerScanline9 && running){
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

    if (line < 192){
        gpuA.renderScanline(line);
        gpuB.renderScanline(line);
        fireHBlank();
    }
}

void NDS::fireVBlank(){
    bus.irq9.raise(IRQ::VBlank);
    bus.irq7.raise(IRQ::VBlank);

    frontend.presentFrame(
        gpuA.frameBuffer.data(), 
        gpuB.frameBuffer.data()
    );   
}

void NDS::fireHBlank(){
    bus.irq9.raise(IRQ::HBlank);
    bus.irq7.raise(IRQ::HBlank); 
}
