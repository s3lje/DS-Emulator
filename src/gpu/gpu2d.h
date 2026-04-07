#pragma once
#include <cstdint>
#include <array>

class Bus;

class GPU2D {
    private:
        Bus* bus;
        bool engineB;

        uint32_t ioBase;
        
        // read register for this engine
        uint16_t readReg16(uint32_t offset);
        uint32_t readReg32(uint32_t offset);
        
        uint32_t bgVramBase()  const { return engineB ? 0x06200000 : 0x06000000; }
        uint32_t objVramBase() const { return engineB ? 0x06600000 : 0x06400000; }
        
        void renderTiledBG (int bg, int y, uint16_t* line);
        void renderAffineBG(int bg, int y, uint16_t* line);
        void renderBitmapBG(int bg, int y, uint16_t* line);
        void renderOBJ     (int y, uint16_t* line);

        static constexpr uint16_t TRANSPARENT = 0x8000;

        uint16_t readPalette(uint32_t offset);
        uint8_t  readVRAM8  (uint32_t addr);
        uint16_t readVRAM16 (uint32_t addr); 
};
