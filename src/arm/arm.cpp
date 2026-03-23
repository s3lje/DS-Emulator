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

bool ARM::checkCond(uint32_t cond){
    switch (cond) {
        case 0x0: return  flagZ();                          // EQ = equal
        case 0x1: return !flagZ();                          // NE = not squal
        case 0x2: return  flagC();                          // CS = carry set
        case 0x3: return !flagC();                          // CC = carry clear
        case 0x4: return  flagN();                          // MI = minus
        case 0x5: return !flagN();                          // PL = plus
        case 0x6: return  flagV();                          // VS = overflow set
        case 0x7: return !flagV();                          // VC = overflow clear
        case 0x8: return  flagC() && !flagZ();              // HI = unsigned higher
        case 0x9: return !flagC() ||  flagZ();              // LS = unsigned lower or same
        case 0xA: return  flagN() ==  flagV();              // GE = signed >=
        case 0xB: return  flagN() !=  flagV();              // LT = signed 
        case 0xC: return !flagZ() && (flagN() == flagV());  // GT = signed >
        case 0xD: return  flagZ() || (flagN() != flagV());  // LE = signed <=
        case 0xE: return true;                              // AL = always
        default:  return false;                             // NV = never
    }
}


