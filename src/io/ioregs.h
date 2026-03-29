#pragma once
#include <cstdint>

// --- Display --- 
static constexpr uint32_t DISPCNT_A  = 0x04000000; // display controller
static constexpr uint32_t DISPCNT_B  = 0x04001000; // -- ;; -- 
static constexpr uint32_t DISPSTAT   = 0x04000004; // display status + IRQ flags
static constexpr uint32_t VCOUNT     = 0x04000006; // Current scanline

// --- Interrupts (ARM9) --- 
static constexpr uint32_t IME9       = 0x04000208; // Interrupts master enable
static constexpr uint32_t IE9        = 0x04000210; // enable mask
static constexpr uint32_t IF9        = 0x04000214; // flags (pending)

// --- Interrupts (ARM7) --- 
static constexpr uint32_t IME7       = 0x04000208; // Same offsets, different bus
static constexpr uint32_t IE7        = 0x04000210;
static constexpr uint32_t IF7        = 0x04000214;

// --- IPC ---
static constexpr uint32_t IPCSYNC    = 0x04000180;
static constexpr uint32_t IPCFIFOCNT = 0x04000184;
static constexpr uint32_t IPCFIFOSEND= 0x04000188;
static constexpr uint32_t IPCFIFORECV= 0x04100000;

// --- Timers ---
static constexpr uint32_t TM0CNT_L  = 0x04000100;
static constexpr uint32_t TM0CNT_H  = 0x04000102;
// TM1-TM3 follow at +4 offsets each

// --- DMA ---
static constexpr uint32_t DMA0SAD   = 0x040000B0;
// DMA1-DMA3 follow at +12 offsets each

// --- Keypad ---
static constexpr uint32_t KEYINPUT  = 0x04000130;
static constexpr uint32_t KEYCNT    = 0x04000132;

// Interrupt flag bits in IE/IF
namespace IRQ {
    static constexpr uint32_t VBlank    = 1 << 0;
    static constexpr uint32_t HBlank    = 1 << 1;
    static constexpr uint32_t VCountMatch = 1 << 2;
    static constexpr uint32_t Timer0    = 1 << 3;
    static constexpr uint32_t Timer1    = 1 << 4;
    static constexpr uint32_t Timer2    = 1 << 5;
    static constexpr uint32_t Timer3    = 1 << 6;
    static constexpr uint32_t DMA0      = 1 << 8;
    static constexpr uint32_t IPC_Sync  = 1 << 16;
    static constexpr uint32_t IPC_SendEmpty = 1 << 17;
    static constexpr uint32_t IPC_RecvNotEmpty = 1 << 18;
    static constexpr uint32_t GameCard  = 1 << 19;
}

