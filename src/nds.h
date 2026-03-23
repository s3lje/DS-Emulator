#pragma once
#include <string>
#include "memory/rom.h"
#include "memory/bus.h"
#include "arm/arm.h"

class NDS {
    public:
        bool loadROM(const std::string& path);
        void run();

    private:
        NDSHeader header {};
        Bus bus;
        ARM arm9 {&bus, true};  // ARM9 - main CPU
        ARM arm7 {&bus, false}; // ARM7 - sub CPU
};
