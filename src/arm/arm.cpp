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
        case 0x0: result = op1 & op2; break; // AND
        case 0x1: result = op1 ^ op2; break; // XOR
        case 0x2:                            // SUB
            result = op1 - op2;
            c = op1 >= op2;
            v = ((op1 ^ op2) & (op1 ^ result)) >> 31;
            break;
        case 0x3:                            // Reverse SUB
            result = op2 - op1;
            c = op2 >= op1;
            v = ((op2 ^ op1) & (op2 ^ result)) >> 31; 
            break;
        case 0x4:                            // ADD
            result = op1 + op2;
            c = result < op1; 
            v = (~(op1 ^ op2) & (op1 ^ result)) >> 31;
            break;
        case 0x5:                            // ADD with carry
            result = op1 + op2 + flagC();
            c = (uint64_t)op1 + op2 + flagC() > 0xFFFFFFFF;
            v = (~(op1 ^ op2) & (op1 ^ result)) >> 31;
            break;
        case 0x6:
            result = op1 - op2 - !flagC();
            c = (uint64_t)op1 >= (uint64_t)op2 + !flagC();
            v = ((op1 ^ op2) & (op1 ^ result)) >> 31;
            break;
        case 0x7:
            result = op2 - op1 - !flagC();
            c = (uint64_t)op2 >= (uint64_t)op1 + !flagC();
            v = ((op2 ^ op1) & (op2 ^ result)) >> 31;
            break;
        case 0x8:                              // TST (AND but only set flags)
            result = op1 & op2;
            updateFlags = true; rd = 16; break;
        case 0x9:                              // TEQ (XOR but only set flags)
            result = op1 ^ op2;
            updateFlags = true; rd = 16; break;
        case 0xA:                              // CMP (SUB but only set flags)
            result = op1 - op2;
            c = op1 >= op2;
            v = ((op1 ^ op2) & (op1 ^ result)) >> 31;
            updateFlags = true; rd = 16; break;
        case 0xB:                              // CMN (ADD but only set flags)
            result = op1 + op2;
            c = result < op1;
            v = (~(op1 ^ op2) & (op1 ^ result)) >> 31;
            updateFlags = true; rd = 16; break;
        case 0xC: result = op1 | op2;  break; // ORR
        case 0xD: result = op2;        break; // MOV
        case 0xE: result = op1 & ~op2; break; // BIC (bit clear)
        case 0xF: result = ~op2;       break; // MVN (move NOT)
    }

    n = result >> 31;
    z = result == 0;

    if (updateFlags){
        if (rd == 15){
            cpsr = currentSPSR();
            flushPipeline();
        } else {
            setNZCV(n, z, c, v);
        }
    }
}

void ARM::execLoadStore(uint32_t instr){
    bool     pre        = (instr >> 24) & 1;
    bool     up         = (instr >> 23) & 1;
    bool     byteAccess = (instr >> 22) & 1;
    bool     wb         = (instr >> 21) & 1;
    bool     load       = (instr >> 20) & 1;
    uint32_t rn         = (instr >> 16) & 0xF;
    uint32_t rd         = (instr >> 12) & 0xF;

    // Decode offset
    uint32_t offset;
    if ((instr >> 25) & 1){
        // Register with optinal shift
        bool dummy;
        uint32_t rm = instr & 0xF;
        offset = barrelShift(r[rm], (instr >> 5) & 3, (instr >> 7) & 0x1F, dummy);
    } else {
        //12bit immediate offset
        offset = instr & 0xFFF;
    }

    uint32_t base = r[rn];

    if (pre) base = up ? base + offset : base - offset; // Apply offset before access

    if (load){
        uint32_t val = byteAccess ? bus->read8(base) : bus->read32(base);
        r[rd] = val;
        if (rd == 15) flushPipeline(); 
    } else {
        uint32_t val = r[rd];
        if (rd == 15) val += 4;
        byteAccess ? bus->write8(base, val & 0xFF) : bus->write32(base, val); 
    }

    // apply offset after access and writeback if preindexed
    if (!pre) base = up ? base + offset : base - offset;
    if (wb || !pre) r[rn] = base;
}

void ARM::execLoadStoreHalf(uint32_t instr){
    bool     pre  = (instr >> 24) & 1;
    bool     up   = (instr >> 23) & 1; 
    bool     imm  = (instr >> 22) & 1;
    bool     wb   = (instr >> 21) & 1;
    bool     load = (instr >> 20) & 1;
    uint32_t rn   = (instr >> 16) & 0xF;
    uint32_t rd   = (instr >> 12) & 0xF;
    uint32_t sh   = (instr >>  5) & 3;    // 01=UH, 10=SB, 11=SH

    uint32_t offset = imm
        ? ((instr >> 4) & 0xF0) | (instr & 0xF)
        : r[instr & 0xF]; 

    uint32_t base = r[rn];
    if (pre) base = up ? base + offset : base - offset;

    if (load){
        uint32_t val;
        switch (sh){
            case 1: val = bus->read16(base); break;
            case 2: val = (int32_t)(int8_t) bus->read8 (base); break;
            case 3: val = (int32_t)(int16_t)bus->read16(base); break;
            default: val = 0;
        }
        r[rd] = val;
        if (rd == 15) flushPipeline();
    } else {
        if (sh == 1) bus->write16(base, r[rd] & 0xFFFF); // STRH
    }

    if (!pre) base = up ? base + offset : base - offset;
    if (wb || !pre) r[rn] = base;
}


void ARM::execBlockTransfer(uint32_t instr){
    bool     pre    = (instr >> 24) & 1;
    bool     up     = (instr >> 23) & 1;
    bool     s      = (instr >> 22) & 1;
    bool     wb     = (instr >> 21) & 1;
    bool     load   = (instr >> 20) & 1;
    uint32_t rn     = (instr >> 16) & 0xF;
    uint16_t rlist  = instr & 0xFFFF;

    int count = __builtin_popcount(rlist);          // count how many registers are being trasferred.
    uint32_t base = r[rn];                          // When down (up=0), start address is base minus
    uint32_t addr = up ? base : base - count * 4;   // total transfer size.

    for (int i = 0; i <= 15; i++){
        if (!(rlist & (1 << i))) continue;

        // preindex, access at addr+4
        uint32_t ea = pre ? addr + 4 : addr;
        addr += 4;

        if (load) {
            r[i] = bus->read32(ea);
            if (i == 15){                           // loading into PC
                flushPipeline();
                if (s) 
                    cpsr = currentSPSR();           // s bit with r15 means restore csps
            }   
        } else {
            uint32_t val = (i == 15) ? r[15] + 4 : r[i];
            bus->write32(ea, val);
        }
    }
    // Write back the updated base register
    // note: if rn is in rlist and its a load, the loaded val takes priority
    if (wb && !(load && (rlist & (1 << rn)))){
        r[rn] = up ? base + count * 4 : base - count * 4;
    }
}

void ARM::execMultiply(uint32_t instr){
    bool     accumulate = (instr >> 21) & 1;
    bool     setFlags   = (instr >> 20) & 1;
    bool     longMul    = (instr >> 23) & 1; //64bit result
    bool     sign       = (instr >> 22) & 1; // signed or unsigned

    uint32_t rd  = (instr >> 16) & 0xF; // long or short
    uint32_t rn  = (instr >> 12) & 0xF; // accumulate register / low result
    uint32_t rs  = (instr >> 8)  & 0xF;
    uint32_t rm  = instr         & 0xF;

    if (!longMul){
        // MUL / MLA, 32bit result
        uint32_t result = r[rm] * r[rs];
        if (accumulate)
            result += r[rn];
        r[rd] = result;
        if (setFlags){
            setFlagN(result >> 31);
            setFlagZ(result == 0); 
        }
    } else {
        // UMULL / UMLAL / SMULL / SMLAL, 64bit result
        uint64_t result;
        if (sign){
            result = (int64_t)(int32_t)r[rm] * (uint32_t)r[rs];
        } else {
            result = (uint64_t)r[rm] * r[rs];
        }
        if (accumulate)
            result += ((uint64_t)r[rd] << 32) | r[rn]; // UMLAL /SMLAL

        r[rn] = result & 0xFFFFFFFF; // low 32 bits
        r[rd] = result >> 32;        // high 32 bits
        if (setFlags){
            setFlagN(result >> 63);
            setFlagZ(result == 0);
        }
    }
}

void ARM::execMSR_MRS(uint32_t instr){
    bool useSPSR = (instr >> 22) & 1;
    bool isMSR   = (instr >> 21) & 1;

    if (!isMSR){
        // move status register to general registr
        uint32_t rd = (instr >> 12) & 0xF;
        r[rd] = useSPSR ? currentSPSR() : cpsr;
    } else {
        // move register or immediate to status register
        uint32_t fieldMask = (instr >> 16) & 0xF; // field mask [19:16] controls
        uint32_t val;                             // which bytes get updated.

        if ((instr >> 25) & 1){
            // immediate val with rotation
            uint32_t imm = instr & 0xFF;
            uint32_t rot = ((instr >> 8) & 0xF) * 2;
            val = (imm >> rot) | (imm << (32 - rot));
        } else {
            val = r[instr & 0xF];
        }

        uint32_t mask = 0;
        if (fieldMask & 1) mask |= 0x000000FF; // control byte
        if (fieldMask & 2) mask |= 0x0000FF00; // extension byte
        if (fieldMask & 4) mask |= 0x00FF0000; // status byte
        if (fieldMask & 8) mask |= 0xFF000000; // flags byte

        if (useSPSR){
            currentSPSR() = (currentSPSR() & ~mask) | (val & mask);
        } else {
            cpsr = (cpsr & ~mask) | (val & mask); // can change CPU mode
        }
    }
}

void ARM::execTHUMB(uint16_t instr){
    uint32_t op = instr >> 12;

    switch (op) {
        case 0x0: case 0x1: {
            // LSL/LSR/ASR Rd, Rs, #imm5
            uint32_t shiftOp = (instr >> 11) & 3;
            uint32_t amount  = (instr >>  6) & 0x1F;
            uint32_t rs      = (instr >>  3) & 7;
            uint32_t rd      = instr         & 7;
            bool carry;
            r[rd] = barrelShift(r[rs], shiftOp, amount, carry);
            setFlagN(r[rd] >> 31);
            setFlagZ(r[rd] == 0);
            if (amount) setFlagC(carry);
            break;
        }
        
        case 0x2: case 0x3: {
            // Move/compare/add/subtract immediate
            uint32_t subop = (instr >> 11) & 3;
            uint32_t rd    = (instr >>  8) & 7;
            uint32_t imm   = instr         & 0xFF;
            uint32_t res;
            bool c = flagC(), v = flagV();
            switch (subop){
                case 0: res = imm; break;
                case 1: res = r[rd] - imm;
                        c = r[rd] >= imm;
                        v = ((r[rd] ^ imm) & (r[rd] ^ res)) >> 31;
                        setNZCV(res>>31, res==0, c, v); return;
                case 2: res = r[rd] + imm;
                        c = res < r[rd];
                        v = (~(r[rd] ^ imm) & (r[rd] ^ res)) >> 31;
                        break;
                case 3: res = r[rd] - imm;
                        c = r[rd] >= imm;
                        v = ((r[rd] ^ imm) & (r[rd] ^ res)) >> 31; 
                        break;
            }
            r[rd] = res;
            setNZCV(res>>31, res==0, c, v);
            break;
        }

        case 0x4: {
            if ((instr >> 11) & 1) {
                // PC-relative load: LDR Rd, [PC, #imm8*4]
                uint32_t rd  = (instr >> 8) & 7;
                uint32_t off = (instr & 0xFF) * 4;
                uint32_t base = (r[15] & ~3u) + off;  // PC is word-aligned here
                r[rd] = bus->read32(base);
            } else {
                // ALU operations and hi-register ops
                uint32_t subop = (instr >> 6) & 0xF;
                uint32_t rs    = (instr >> 3) & 7;
                uint32_t rd    =  instr       & 7;

                // Hi-register operations (bit 6-7 can address r8-r15)
                if (subop >= 0x8) {
                    uint32_t h1 = (instr >> 7) & 1;
                    uint32_t h2 = (instr >> 6) & 1;
                    rs = (instr >> 3) & 0xF;  // full 4-bit index
                    rd = ((instr & 7) | (h1 << 3));
                    switch (subop & 3) {
                        case 0: r[rd] += r[rs]; if(rd==15) flushPipeline(); break; // ADD
                        case 1: { uint32_t res = r[rd]-r[rs];
                                  setNZCV(res>>31,res==0,r[rd]>=r[rs],
                                  ((r[rd]^r[rs])&(r[rd]^res))>>31); break; } // CMP
                        case 2: r[rd] = r[rs]; if(rd==15) flushPipeline(); break; // MOV
                        case 3: // BX/BLX
                            if (h1) r[14] = r[15] - 2;  // BLX saves return address
                            cpsr = (cpsr & ~(1<<5)) | ((r[rs] & 1) << 5);
                            r[15] = r[rs] & ~1u;
                            flushPipeline(); break;
                    }
                    break;
                }

                // Standard ALU ops
                bool carry; uint32_t res;
                switch (subop) {
                    case 0x0: r[rd] &= r[rs]; setFlagN(r[rd]>>31); setFlagZ(r[rd]==0); break; // AND
                    case 0x1: r[rd] ^= r[rs]; setFlagN(r[rd]>>31); setFlagZ(r[rd]==0); break; // EOR
                    case 0x2: res = barrelShift(r[rd],0,r[rs]&0xFF,carry);
                              r[rd]=res; setFlagN(res>>31);setFlagZ(res==0);setFlagC(carry); break; // LSL
                    case 0x3: res = barrelShift(r[rd],1,r[rs]&0xFF,carry);
                              r[rd]=res; setFlagN(res>>31);setFlagZ(res==0);setFlagC(carry); break; // LSR
                    case 0x4: res = barrelShift(r[rd],2,r[rs]&0xFF,carry);
                              r[rd]=res; setFlagN(res>>31);setFlagZ(res==0);setFlagC(carry); break; // ASR
                    case 0x5: res=r[rd]+r[rs]+flagC();
                              setNZCV(res>>31,res==0,(uint64_t)r[rd]+r[rs]+flagC()>0xFFFFFFFF,
                              (~(r[rd]^r[rs])&(r[rd]^res))>>31); r[rd]=res; break; // ADC
                    case 0x7: res = barrelShift(r[rd],3,r[rs]&0xFF,carry);
                              r[rd]=res; setFlagN(res>>31);setFlagZ(res==0);setFlagC(carry); break; // ROR
                    case 0x8: res=r[rd]&r[rs]; setFlagN(res>>31);setFlagZ(res==0); break; // TST
                    case 0x9: res=0-r[rs]; r[rd]=res;
                              setNZCV(res>>31,res==0,r[rs]==0,((r[rs])&res)>>31); break; // NEG
                    case 0xA: res=r[rd]-r[rs];
                              setNZCV(res>>31,res==0,r[rd]>=r[rs],
                              ((r[rd]^r[rs])&(r[rd]^res))>>31); break; // CMP
                    case 0xC: r[rd] |= r[rs]; setFlagN(r[rd]>>31);setFlagZ(r[rd]==0); break; // ORR
                    case 0xD: r[rd]*=r[rs]; setFlagN(r[rd]>>31);setFlagZ(r[rd]==0); break; // MUL
                    case 0xE: r[rd]&=~r[rs];setFlagN(r[rd]>>31);setFlagZ(r[rd]==0); break; // BIC
                    case 0xF: r[rd]=~r[rs]; setFlagN(r[rd]>>31);setFlagZ(r[rd]==0); break; // MVN
                }
            }
            break;
        }
        case 0x5: {
            // Load/Store with register offset
            uint32_t subop = (instr >> 9) & 7; 
            uint32_t ro    = (instr >> 6) & 7;
            uint32_t rb    = (instr >> 3) & 7;
            uint32_t rd    =  instr       & 7; 
            uint32_t addr  = r[rb] + r[ro];
            switch (subop){
                case 0: bus->write32(addr, r[rd]); break;
                case 1: bus->write16(addr, r[rd]); break;
                case 2: bus->write8 (addr, r[rd]); break;
                case 3: r[rd]=(int32_t)(int8_t) bus->read8 (addr); break;
                case 4: r[rd]=bus->read32(addr);                   break;
                case 5: r[rd]=bus->read16(addr);                   break;
                case 6: r[rd]=bus->read8(addr);                    break;
                case 7: r[rd]=(int32_t)(int16_t)bus->read16(addr); break;
            }
            break;
        }
        case 0x6: case 0x7: case 0x8: {
            // Load/Store with 5bit immediate offset
            bool     load  = (instr >> 11) & 1;
            bool     byteA = (op == 0x7);
            bool     half  = (op == 0x8);
            uint32_t off   = (instr >> 6) & 0x1F;
            uint32_t rb    = (instr >> 3) & 7;
            uint32_t rd    =  instr       & 7;
            uint32_t addr  = r[rb] + (half ? off*2 : byteA ? off : off*4); 
            if (load) r[rd] = byteA ? bus->read8(addr)
                            : half  ? bus->read16(addr)
                                    : bus->read32(addr); 
            else { 
                if (byteA) bus->write8(addr, r[rd]);
                else if (half) bus->write16(addr, r[rd]);
                else bus->write32(addr, r[rd]);
            }
            break;
        }
        case 0x9: {
            // SP-relative Load/Store
            bool     sp = (instr >> 11) & 1;
            uint32_t rd = (instr >> 8)  & 7;
            uint32_t off = (instr & 0xFF) * 4;
            r[rd] = (sp ? r[13] : (r[15] & ~3u)) + off;
            break;
        }
        case 0xA: {
            // Load address (PC or SP relative into register)
            bool     sp = (instr >> 11) & 1;
            uint32_t rd = (instr >> 8)  & 7;
            uint32_t off = (instr & 0xFF) * 4;
            r[rd] = (sp ? r[13] : (r[15] & ~3u)) + off;
            break;
        }
        case 0xB: {
            // Push/pop and SP adjustment
            if ((instr >> 8) & 1) {
                // PUSH / POP
                bool     pop   = (instr >> 11) & 1;
                bool     lrpc  = (instr >>  8) & 1;  // push LR / pop PC
                uint8_t  rlist =  instr & 0xFF;
                if (!pop) {
                    // PUSH: pre-decrement SP for each register
                    int count = __builtin_popcount(rlist) + lrpc;
                    r[13] -= count * 4;
                    uint32_t addr = r[13];
                    for (int i = 0; i < 8; i++) {
                        if (rlist & (1<<i)) { bus->write32(addr, r[i]); addr+=4; }
                    }
                    if (lrpc) bus->write32(addr, r[14]);
                } else {
                    // POP: post-increment SP
                    uint32_t addr = r[13];
                    for (int i = 0; i < 8; i++) {
                        if (rlist & (1<<i)) { r[i]=bus->read32(addr); addr+=4; }
                    }
                    if (lrpc) { r[15]=bus->read32(addr); addr+=4; flushPipeline(); }
                    r[13] = addr;
                }
            } else {
                // ADD/SUB SP, #imm7
                uint32_t off = (instr & 0x7F) * 4;
                r[13] = (instr & 0x80) ? r[13] - off : r[13] + off;
            }
            break;
        }
        case 0xC: {
            // LDMIA / STMIA — load/store multiple with writeback
            bool     load  = (instr >> 11) & 1;
            uint32_t rb    = (instr >>  8) & 7;
            uint8_t  rlist =  instr & 0xFF;
            uint32_t addr  = r[rb];
            for (int i = 0; i < 8; i++) {
                if (!(rlist & (1<<i))) continue;
                if (load)
                    r[i] = bus->read32(addr);
                else
                    bus->write32(addr, r[i]);
                addr += 4;
            }
            r[rb] = addr;  // always write back in THUMB LDM/STM
            break;
        }
        case 0xD: {
            // Conditional branch / SWI
            uint32_t cond = (instr >> 8) & 0xF;
            if (cond == 0xF) {
                execSWI(0); // SWI
            } else if (checkCond(cond)) {
                int32_t off = (int8_t)(instr & 0xFF);
                r[15] += off * 2;
                flushPipeline();
            }
            break;
        }
        case 0xE: {
            // Unconditional branch
            int32_t off = ((int32_t)(instr << 21)) >> 20;  // sign extend 11-bit
            r[15] += off;
            flushPipeline();
            break;
        }
        case 0xF: {
            // BL / BLX — two-instruction sequence for long branches.
            // First instruction (H=0): LR = PC + (offset_hi << 12)
            // Second instruction (H=1): PC = LR + (offset_lo << 1), LR = old PC | 1
            bool h = (instr >> 11) & 1;
            if (!h) {
                int32_t off = ((int32_t)(instr << 21)) >> 9;  // sign extend
                r[14] = r[15] + off;
            } else {
                uint32_t target = r[14] + ((instr & 0x7FF) << 1);
                r[14] = (r[15] - 2) | 1;  // return address with THUMB bit
                r[15] = target;
                flushPipeline();
            }
            break;
        }
    }
}

// Stubbed instruction handlers
void ARM::execSWI(uint32_t instr)            {}

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
