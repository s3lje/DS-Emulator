#include "bus.h"
#include <iostream>

uint32_t Bus::read32(uint32_t addr){
    addr &= ~3u;    // Making sure of 32bit alignment

    switch (addr >> 24){
        case 0x02:
            {
                uint32_t off = (addr - 0x02000000) & 0x3FFFFF;
                return *reinterpret_cast<uint32_t*>(&mainRAM[off]);
            }
        case 0x03:
            {
                uint32_t off = (addr - 0x03000000) & 0x7FFF;
                return *reinterpret_cast<uint32_t*>(&sharedWRAM[off]);
            }
        case 0x04:
            {
                return readIO32(addr);
            }
        case 0x08: case 0x09:
            if (addr - 0x08000000 < rom.size())
                return *reinterpret_cast<uint32_t*>(&rom[addr - 0x08000000]);
            return 0xFFFFFFFF; // open bus
        default:
            return 0;   //Unmapped memory
    }
}

uint8_t Bus::read8(uint32_t addr){
    uint32_t word = read32(addr & ~3u);
    return (word >> ((addr & 3) * 8)) & 0xFF;
}

uint16_t Bus::read16(uint32_t addr){
    addr &= ~1u;    // Force 16bit alignment
    uint32_t word = read32(addr & ~3u);
    return (word >> ((addr & 2) * 2)) & 0xFFFF;
}


void Bus::write8 (uint32_t addr, uint8_t  val) {
    uint32_t off = addr & ~3u;
    uint32_t word = read32(off);
    int      shift = (addr & 2) * 8;
    word = (word & ~(0xFFFFu << shift)) | (uint32_t(val) << shift);
    write32(off, word);
}

void Bus::write16(uint32_t addr, uint16_t val) {
    addr &= ~1u;
    uint32_t off  = addr & ~3u;
    uint32_t word = read32(off);
    int      shift = (addr & 2) * 8;
    word = (word & ~(0xFFFFu << shift)) | (uint32_t(val) << shift);
    write32(off, word);
}

void Bus::write32(uint32_t addr, uint32_t val) {
    addr &= ~3u;
    switch (addr >> 24){
        case 0x02: {
                       uint32_t off = (addr - 0x02000000) & 0x3FFFFF;
                       *reinterpret_cast<uint32_t*>(&mainRAM[off]) = val;
                       break;
                   }
        case 0x03: {
                       uint32_t off = (addr - 0x03000000) & 0x7FFF;
                       *reinterpret_cast<uint32_t*>(&sharedWRAM[off]) = val;
                       break; 
                   }
        case 0x04:
                   writeIO32(addr, val);
                   break;
        default:
                   break;
    }
}

uint32_t Bus::readIO32(uint32_t addr){
    // Stubbed
    return 0;
}

void Bus::writeIO32(uint32_t addr, uint32_t val){
    //Stubbed
}
