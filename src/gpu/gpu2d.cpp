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

void GPU2D::renderTiledBG(int bg, int y, uint16_t* line){
    // control registers and scroll registers for BGs
    uint16_t bgcnt   = readReg16(0x008 + bg * 2);
    uint16_t scrollX = readReg16(0x010 + bg * 4);
    uint16_t scrollY = readReg16(0x012 + bg * 4);

    bool     is8bpp   = (bgcnt >> 7) & 1;
    uint32_t tileBase = ((bgcnt >> 2) & 0x3) * 0x4000;
    uint32_t mapBase  = ((bgcnt >> 8) & 0x1F) * 0x800;

    uint32_t px0 = scrollX & 0x1FF;
    uint32_t py0 = (scrollY + y) & 0x1FF;
    
    // each entry is 16 bits
    uint32_t tilemapY = py0 / 8;
    uint32_t pixelY   = py0 % 8;

    for (int x = 0; x < 256; x++){
        uint32_t px = (px0 + x) & 0x1FF;
        uint32_t tilemapX = px / 8;
        uint32_t pixelX   = px % 8;

        // caluclate the current screen block
        uint32_t screenBlock = 0;
        uint32_t screenSize  = (bgcnt >> 14) & 3;
        if (screenSize == 1 && tilemapX >= 32) screenBlock = 1;
        if (screenSize == 2 && tilemapY >= 32) screenBlock = 1;
        if (screenSize == 3){
            if (tilemapX >= 32) screenBlock += 1;
            if (tilemapY >= 32) screenBlock += 2; 
        }

        uint32_t mapAddr = bgVramBase() + mapBase
                         + screenBlock * 0x800
                         + (tilemapY & 31) * 32 * 2
                         + (tilemapX & 31) * 2; 

        uint16_t entry   = readVRAM16(mapAddr);
        uint32_t tileNum = entry & 0x3FF;
        bool     flipH   = (entry >> 10) & 1;
        bool     flipV   = (entry >> 11) & 1;
        uint32_t palette = (entry >> 12) & 0xF;

        uint32_t tx = flipH ? 7 - pixelX : pixelX;
        uint32_t ty = flipV ? 7 - pixelY : pixelY;

        uint8_t colorIdx;
        if (is8bpp) {
            // 1 byte per pixel
            uint32_t tileAddr = bgVramBase() + tileBase + tileNum * 64 + ty * 8 + tx;
            colorIdx = readVRAM8(tileAddr); 
        }
    }
}
