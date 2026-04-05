#pragma once
#include <cstdint>
#include "interrupts.h"
#include "ioregs.h"

struct Timer {
    uint16_t counter    = 0;
    uint16_t reload     = 0;
    uint8_t  control    = 0;

    bool enabled()   const { return (control >> 7) & 1; }
    bool cascade()   const { return (control >> 2) & 1; } // count on prev overflow
    bool irqEnable() const { return (control >> 6) & 1; }

    int prescaler() const {
        static const int rates[] = {1, 64, 256, 1024};
        return rates[control & 3]; 
    }
};

struct TimerGroup {
    Timer timers[4];
    int   cycleCounts[4] = {};

    uint8_t tick(int cycles, InterruptController& irq){
        uint8_t overflowed = 0; 

        for (int i = 0; i < 4; i++){
            Timer& t = timers[i];
            if (!t.enabled()) continue;

            if (t.cascade()){
                // only increment when previous timer overflowed
                if (i > 0 && (overflowed & (1 << (i-1)))){
                    if (++t.counter == 0){
                        t.counter = t.reload;
                        overflowed |= (1 << i);
                        if (t.irqEnable())
                            irq.raise(IRQ::Timer0 << i);
                    }
                }
            } else {
                // increment every N cycles
                cycleCounts[i] += cycles;
                int rate =t.prescaler();
                while (cycleCounts[i] >= rate){
                    cycleCounts[i] -= rate;
                    if (++t.counter == 0){
                        t.counter = t.reload;
                        overflowed |= (1 << i);
                        if (t.irqEnable())
                            irq.raise(IRQ::Timer0 << i);
                    }
                }
            }
        }
        return overflowed; 
    }
};
