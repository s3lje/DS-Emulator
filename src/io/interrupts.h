#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#pragma once
#include <cstdint>

// 1 of these per cpu 
struct InterruptController {
    uint32_t IME = 0; // master enable 
    uint32_t IE  = 0; // which interrupts are enabled
    uint32_t IF  = 0; // which interrupts are pending

    // hardware interrupt
    void raise(uint32_t flag){
        IF |= flag;
    }

    // IRQ firing
    bool pending() const {
        return (IME & 1) && (IE & IF);
    }

    // hardware acknowledge
    void acknowledgeIF(uint32_t val){
        IF &= ~val; 
    }
};

#endif
