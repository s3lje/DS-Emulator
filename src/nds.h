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
        void runFrame();
        void runScanline(int line);
        void fireVBlank();
        void fireHBlank();

            
        NDSHeader header;
        Bus bus;
        ARM arm9 {&bus, &bus.irq9, true};  // ARM9 - main CPU
        ARM arm7 {&bus, &bus.irq7, false}; // ARM7 - sub CPU
    

        // Apparently the ARM9 runs at around 67 MHz and ARM7 at 33
        // according to some random website atleast so i might have to change this later
        static constexpr int CyclesPerScanline9 = 2130;
        static constexpr int CyclesPerScanline7 = 1065; // half speed
};
