#include "nds.h"
#include <iostream>
#include <fstream>
#include <cstring>


bool NDS::loadROM(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    f.seekg(0, std::ios::end);
    rom.resize(f.tellg());
    f.seekg(0);
    f.read(reinterpret_cast<char*>(rom.data()), rom.size());

    header = *reinterpret_cast<NDSHeader*>(rom.data());

    std::cout << "Game: " << header.gameTitle << std::endl;
    std::cout << "ARM9: " << header.arm9Size << "bytes @ 0x"
              << std::hex << header.arm9RamAddress << std::endl;
    std::cout << "ARM7: " << header.arm7Size << "bytes @ 0x"
              << std::hex << header.arm7RamAddress << std::endl;

    // Copying ARM9 code from ROM to correct RAM address
    std::memcpy(
        &mainRAM[header.arm9RamAddress - 0x02000000],
        rom.data() + header.arm9RomOffset,
        header.arm9Size
    );

    return true;
}

void NDS::run(){
    // Stub for now... will implement main loop later
    std::cout << "NDS::run() called...\n";
}
