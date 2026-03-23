#include "nds.h"
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
        arm9.step();
        arm7.step();
    }
}
