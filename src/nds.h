#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "memory/rom.h"

class NDS {
    public:
        bool loadROM(const std::string& path);
        void run();

    private:
        NDSHeader header {};
        std::vector<uint8_t> rom;
        std::array<uint8_t, 0x400000> mainRAM {};
};
