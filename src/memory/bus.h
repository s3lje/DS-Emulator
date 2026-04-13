#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "../io/interrupts.h"
#include "../io/timers.h"

class Bus {
    public:
        uint8_t  read8 (uint32_t addr);
        uint16_t read16(uint32_t addr);
        uint32_t read32(uint32_t addr);
        void write8 (uint32_t addr, uint8_t  val);
        void write16(uint32_t addr, uint16_t val);
        void write32(uint32_t addr, uint32_t val);

        std::array<uint8_t, 0x400000> mainRAM   {}; // 4 MB @ 0x02000000
        std::array<uint8_t, 0x8000>   sharedWRAM{}; // 32 KB @ 0x03000000
        std::vector<uint8_t>          rom;          // cartridge ROM
    
        std::array<uint8_t, 0x100000> vramA  {};     // 128KB, BG tiles engine A
        std::array<uint8_t, 0x100000> vramB  {};     // 128KB, BG tiles engine B
        std::array<uint8_t, 0x20000>  vramE  {};     // 64KB,  palette/BG engine B
        std::array<uint8_t, 0x4000>   oam    {};     // 4KB,   sprite attributes
        std::array<uint8_t, 0x800>    pallete{};     // 2KB,   color palettes
    
        InterruptController irq9;
        InterruptController irq7; 

        uint16_t vcount = 0; 
        uint16_t keyinput = 0x3FF;
        
        TimerGroup timers9;
        TimerGroup timers7; 

    private:
        uint32_t readIO32 (uint32_t addr);
        void     writeIO32(uint32_t addr, uint32_t val);

};
