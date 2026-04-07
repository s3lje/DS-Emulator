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

void GPU2D::renderScanline(int y){
    uint16_t line[256];
    uint16_t backdrop = readPalette(0) & 0x7FFF;
    for (int i = 0; i < 256; i++) line[i] = backdrop;

    uint32_t dispcnt = readReg32(0x000);
    uint32_t bgMode  = dispcnt & 7;

    // renders bg back to front
    for (int bg = 3; bg >= 0; bg--){
        if (!(dispcnt & (1 << (8 + bg)))) continue;

        switch (bgMode){
            case 0:
                // Mode 0: all 4 BGs are tiled (most common)
                renderTiledBG(bg, y, line);
                break;
            case 1:
                // Mode 1: BG0/BG1 tiled, BG2 affine
                if (bg < 2) renderTiledBG(bg, y, line);
                else if (bg == 2) renderAffineBG(bg, y, line);
                break;
            case 5:
                // Mode 5: BG2/BG3 as 256x192 bitmap
                if (bg >= 2) renderBitmapBG(bg, y, line);
                break;
            default:
                break;
        }
    }
    
    // sprites on top of backgrounds
    if (dispcnt & (1 << 12))
        renderOBJ(y, line);

    // copy line to buffer
    for (int x = 0; x < 256; x++)
        frameBuffer[y * 256 + x] = line[x]; 
}


