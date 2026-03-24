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

uint32_t ARM::barrelShift(uint32_t val, uint32_t type,
        uint32_t amount, bool& carry){
    // If amount is 0, carry out is the C flag
    if (amount == 0){
        carry = flagC();
        return val;
    }

    switch (type) {
        case 0: // Logical shift left
            if (amount >= 32) {
                carry = (amount == 32) ? (val & 1) : 0;
                return 0;
            }
            carry = (val >> (32 - amount)) & 1;
            return val << amount;


        case 1: // Logical Right Shift
            if (amount >= 32){
                carry = (amount == 32) ? (val & 1) : 0;
                return 0;
            }
            carry = (val >> (amount - 1)) & 1;
            return val >> amount; 
        

        case 2: // Arithmetic shift right
            if (amount >= 32){
                carry = (val >> 31) & 1;
                return (uint32_t)val >> 31;
            }
            carry = (val >> (amount - 1)) & 1;
            return (uint32_t)val >> amount; // cast to signed for extension
        

        case 3: // Rotate right
            amount &= 31;   // Rotation wraps every 32 bits
            if (amount == 0){
                carry = (val >> 31) & 1;
                return val;
            }
            carry = (val >> (amount - 1)) & 1;
            return (val >> amount) | (val << (32 - amount));
    }

    return val; 
}

void ARM::execARM(uint32_t instr){
    uint32_t op     = (instr >> 20) & 0xFF;
    uint32_t bits74 = (instr >> 4)  & 0xF;
    
    if ((instr & 0x0FC000F0) == 0x00000090){
        execMultiply(instr); return; 
    }
    if ((instr & 0x0C000000) == 0x00000000){
        if (bits74 == 0x9 && ((op & 0xF0) != 0)){
            execLoadStoreHalf(instr); return;
        }
        if ((op & 0xFB) == 0x10){
            execMSR_MRS(instr); return;
        }
        execDataProcessing(instr); return;
    }

    if ((instr & 0x0C000000) == 0x04000000) { execLoadStore(instr);     return; }
    if ((instr & 0x0E000000) == 0x08000000) { execBlockTransfer(instr); return; }
    if ((instr & 0x0E000000) == 0x0A000000) { execBranch(instr);         return; }
    if ((instr & 0x0F000000) == 0x0F000000) { execSWI(instr);           return; }

    // skipping undefined instr, will trigger exception later
}   


void ARM::execDataProcessing(uint32_t instr){
    bool     updateFlags = (instr >> 20) & 1;
    uint32_t opcode      = (instr >> 21) & 0xF; 
    uint32_t rn          = (instr >> 16) & 0xF; // first opened register index
    uint32_t rd          = (instr >> 12) & 0xF; // destination register index
    bool     carry       = flagC();             // carry-in for ADC/SBC

    // Decode op2
    uint32_t op2;
    if ((instr >> 25) & 1){
        uint32_t imm       = instr & 0xFF;
        uint32_t rotate = ((instr >> 8) & 0xF) * 2; 
        op2 = (imm >> rotate) | (imm << (32 - rotate));
        if (rotate) carry = (op2 >> 31) & 1; 
    } else {
        uint32_t rm        = instr & 0xF;
        uint32_t shiftType = (instr >> 5) & 3;
        uint32_t shiftAmt;

        if ((instr >> 4) & 1){
            shiftAmt = r[(instr >> 8) & 0xF] & 0xFF;
        } else {
            shiftAmt = (instr >> 7) & 0x1F;
        }
        op2 = barrelShift(r[rm], shiftType, shiftAmt, carry);
    }

    uint32_t op1    = r[rn];
    uint32_t result = 0;
    bool n, z, c = carry, v = flagV();

    switch (opcode){

    }
}

// Stubbed instruction handlers
void ARM::execMultiply(uint32_t instr)       {}
void ARM::execLoadStore(uint32_t instr)      {}
void ARM::execLoadStoreHalf(uint32_t instr)  {}
void ARM::execBlockTransfer(uint32_t instr)  {}
void ARM::execSWI(uint32_t instr)            {}
void ARM::execMSR_MRS(uint32_t instr)        {}
void ARM::execTHUMB(uint16_t instr)          {}

void ARM::execBranch(uint32_t instr){
    // bit 24 distinguishes BL from B
    bool link = (instr >> 24) & 1; 

    uint32_t offset = (uint32_t)(instr << 8) >> 6;  // sign extend + << 2

    if (link)
        r[14] = r[15] - 4;

    r[15] += offset;
    flushPipeline();
}

void ARM::triggerIRQ(){
    if (cpsr & (1 << 7)) return;    // ignore if the I bit is set 

    r13_14_irq[1] = r[15] - (inThumb() ? 2 : 4) + 4; // save return address in LR_irq
    // save current cpsr into spsr_irq 
    spsr_irq = cpsr;                             

    // Switch to IRQ mode
    cpsr = (cpsr & ~0x3Fu) | MODE_IRQ;
    cpsr |=  (1 << 7);
    cpsr &= ~(1 << 5);

    // Jump to the irq vector
    r[15] = isARM9 ? 0x0FFFF0018 : 0x00000018;
    flushPipeline();
}

void ARM::checkInterrupts(){
    // Stub for now
}

uint32_t& ARM::currentSPSR(){
    switch (cpsr & 0x1F){
        case MODE_FIQ: return spsr_fiq;
        case MODE_IRQ: return spsr_irq;
        case MODE_SVC: return spsr_svc;
        case MODE_ABT: return spsr_abt;
        case MODE_UND: return spsr_und;
        default:       return spsr_svc; // shouldnt happen in USER/SYSTEM mode
    }
}
