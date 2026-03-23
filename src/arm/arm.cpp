#include "arm.h"
#include "../memory/bus.h"
#include <stdexcept>

ARM::ARM(Bus* bus, bool isARM9) : bus(bus), isARM9(isARM9){
    // CPSR reset value
    cpsr = MODE_SVC | (1 << 7) | (1 << 6);
}

void ARM::setPC(uint32_t addr){
    r[15] = addr;
    flushPipeline();
}

void ARM::flushPipeline(){
    // After branch, pre-advance PC by 8 
    if (inThumb())
        r[15] = (r[15] & ~1u) + 4;
    else
        r[15] = (r[15] & ~3u) + 8;
}

void ARM::step(){
    if (inThumb()){
        // fetch 16bit instr @ PC-4
        uint16_t instr = bus->read16(r[15] - 4);
        r[15] += 2;
        execTHUMB(instr);
    } else {
        // fetch 32bit instr @ PC-8
        uint32_t instr = bus->read32(r[15] - 8);
        r[15] += 4;
        if (checkCond(instr >> 28)) // Top 4 bits are condition code
            execARM(instr);
    }
}


