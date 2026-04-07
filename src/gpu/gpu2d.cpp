#include "gpu2d.h"
#include "../memory/bus.cpp"
#include "../io/ioregs.h"

GPU2D::GPU2D(Bus* bus, bool engineB) 
           : bus(bus), engineB(engineB),
             ioBase(engineB ? 0x04001000 : 0x04000000) {};

uint16_t GPU2D::readReg16(uint32_t offset){
    return bus->read16(ioBase + offset);
}
uint32_t GPU2D::readReg32(uint32_t offset){
    return bus->read32(ioBase + offset);
}

// fixed regions for VRAM and pallete
uint8_t  GPU2D::readVRAM8 (uint32_t addr) { return bus->read8 (addr);}
uint16_t GPU2D::readVRAM16(uint32_t addr) { return bus->read16(addr);}
uint16_t GPU2D::readPalette(uint32_t offset) {
    uint32_t base = engineB ? 0x05000400 : 0x05000000;
    return bus->read16(base + offset);
}


