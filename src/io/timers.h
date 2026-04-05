#pragma once
#include <cstdint>
#include "interrupts.h"

struct Timer {
    uint16_t counter    = 0;
    uint16_t reload     = 0;
    uint8_t  control    = 0;

    bool enabled()   const { return (control >> 7) & 1; }
    bool cascade()   const { return (control >> 2) & 1; } // count on prev overflow
    bool irqEnable() const { return (control >> 6) & 1; }

    int prescaler() const {
        static const int rates[] = {1, 64, 256, 1024};
        return rates[constrol & 3]; 
    }
};


