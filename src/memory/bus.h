#pragma once
#include <cstdint>
#include <array>
#include <vector>

class Bus {
    public:
        uint8_t  read8 (uint32_t addr);
        uint16_t read16(uint32_t addr);
        uint32_t read32(uint32_t addr);
        void write8 (uint32_t addr, uint8_t  val);
        void write16(uint32_t addr, uint16_t val);
        void write32(uint32_t addr, uint32_t val);

        std::array<uint8_t, 0x400000> mainRAM   {};  // 4 MB @ 0x02000000
        std::array<uint8_t, 0x8000>   sharedWRAM{};  // 32 KB @ 0x03000000
        std::vector<uint8_t>          rom;           // cartridge ROM
    
    private:
        uint32_t readIO32 (uint32_t addr);
        void     writeIO32(uint32_t addr, uint32_t val);

};
