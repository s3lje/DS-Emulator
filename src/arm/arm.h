#ifndef __ARM_H
#define __ARM_H

#pragma once
#include <cstdint>
#include <array>
#include "../io/interrupts.h"

class Bus;

enum CPUMode : uint32_t {
    MODE_USER   = 0x10,
    MODE_FIQ    = 0x11,
    MODE_IRQ    = 0x12,
    MODE_SVC    = 0x13,
    MODE_ABT    = 0x17,
    MODE_UND    = 0x18,
    MODE_SYSTEM = 0x1F
};

class ARM {
    public:
        ARM(Bus* bus, InterruptController* irq, bool isARM9);

        void step();
        void setPC(uint32_t addr);
        void triggerIRQ();
        void checkInterrupts();

        uint32_t r[16] {};
        uint32_t cpsr {};

        // Banked registers
        uint32_t r8_14_fiq[7]  {};
        uint32_t r13_14_irq[2] {};
        uint32_t r13_14_svc[2] {};
        uint32_t r13_14_abt[2] {};
        uint32_t r13_14_und[2] {}; 

        // Saved Program Status Registers
        uint32_t spsr_fiq{}, spsr_irq{}, spsr_svc{};
        uint32_t spsr_abt{}, spsr_und{};


    private:
        Bus* bus;
        InterruptController* irq; 
        bool isARM9;

        // CPSR flag helpers
        bool inThumb() const { return (cpsr >> 5) & 1; }
        bool flagN()   const { return (cpsr >> 31) & 1; }
        bool flagZ()   const { return (cpsr >> 30) & 1; }
        bool flagC()   const { return (cpsr >> 29) & 1; }
        bool flagV()   const { return (cpsr >> 28) & 1; }

        void setFlag(int bit, bool val){
            cpsr = (cpsr & ~(1u << bit)) | (uint32_t(val) << bit);
        }
        void setFlagN(bool v) { setFlag(31, v); }
        void setFlagZ(bool v) { setFlag(30, v); }
        void setFlagC(bool v) { setFlag(29, v); }
        void setFlagV(bool v) { setFlag(28, v); }

        void setNZCV(bool n, bool z, bool c, bool v){
            cpsr = (cpsr & 0x0FFFFFFF)
                 | (uint32_t(n) << 31) | (uint32_t(z) << 30)
                 | (uint32_t(c) << 29) | (uint32_t(v) << 28);
        }

        void flushPipeline();

        uint32_t& currentSPSR();

        bool checkCond(uint32_t cond);

        uint32_t barrelShift(uint32_t val, uint32_t type,
                             uint32_t amount, bool& carryOut);

        // ARM instruction group handlers
        void execARM(uint32_t instr);
        void execDataProcessing(uint32_t instr);
        void execMultiply(uint32_t instr);
        void execLoadStore(uint32_t instr);
        void execLoadStoreHalf(uint32_t instr);
        void execBlockTransfer(uint32_t instr);
        void execBranch(uint32_t instr);
        void execSWI(uint32_t instr);
        void execMSR_MRS(uint32_t instr);

        // THUMB instruction handler
        void execTHUMB(uint16_t instr); 
};

#endif
