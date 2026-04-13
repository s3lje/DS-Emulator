#include "bus.h"
#include "../io/ioregs.h"
#include <iostream>
#include <memory>

uint32_t Bus::read32(uint32_t addr){
    addr &= ~3u;    // Making sure of 32bit alignment

    switch (addr >> 24){
        case 0x02:
            {
                uint32_t off = (addr - 0x02000000) & 0x3FFFFF;
                return *reinterpret_cast<uint32_t*>(&mainRAM[off]);
            }
        case 0x03:
            {
                uint32_t off = (addr - 0x03000000) & 0x7FFF;
                return *reinterpret_cast<uint32_t*>(&sharedWRAM[off]);
            }
        case 0x04:
            {
                return readIO32(addr);
            }

        case 0x05: {    // palette RAM
            uint32_t off = (addr - 0x05000000) & 0x7FF;
            return *reinterpret_cast<uint32_t*>(&pallete[off]);
        }

        case 0x06: {    // VRAM
            uint32_t off = addr - 0x06000000;
            if (off < 0x100000)
                return *reinterpret_cast<uint32_t*>(&vramA[off & 0xFFFFC]);
            off -= 0x100000;
            if (off < 0x100000)
                return *reinterpret_cast<uint32_t*>(&vramB[off & 0xFFFFC]);
            return 0;
        }

        case 0x07: {    // OAM
            uint32_t off = (addr - 0x07000000) & 0x7FF;
            return *reinterpret_cast<uint32_t*>(&oam[off]);
        }

        case 0x08: case 0x09:
            if (addr - 0x08000000 < rom.size())
                return *reinterpret_cast<uint32_t*>(&rom[addr - 0x08000000]);
            return 0xFFFFFFFF; // open bus
        default:
            return 0;   //Unmapped memory
    }
}

uint8_t Bus::read8(uint32_t addr){
    uint32_t word = read32(addr & ~3u);
    return (word >> ((addr & 3) * 8)) & 0xFF;
}

uint16_t Bus::read16(uint32_t addr){
    addr &= ~1u;    // Force 16bit alignment
    uint32_t word = read32(addr & ~3u);
    return (word >> ((addr & 2) * 2)) & 0xFFFF;
}


void Bus::write8 (uint32_t addr, uint8_t  val) {
    uint32_t off = addr & ~3u;
    uint32_t word = read32(off);
    int      shift = (addr & 2) * 8;
    word = (word & ~(0xFFFFu << shift)) | (uint32_t(val) << shift);
    write32(off, word);
}

void Bus::write16(uint32_t addr, uint16_t val) {
    addr &= ~1u;
    uint32_t off  = addr & ~3u;
    uint32_t word = read32(off);
    int      shift = (addr & 2) * 8;
    word = (word & ~(0xFFFFu << shift)) | (uint32_t(val) << shift);
    write32(off, word);
}

void Bus::write32(uint32_t addr, uint32_t val) {
    addr &= ~3u;
    switch (addr >> 24){
        case 0x02: {
                       uint32_t off = (addr - 0x02000000) & 0x3FFFFF;
                       *reinterpret_cast<uint32_t*>(&mainRAM[off]) = val;
                       break;
                   }
        case 0x03: {
                       uint32_t off = (addr - 0x03000000) & 0x7FFF;
                       *reinterpret_cast<uint32_t*>(&sharedWRAM[off]) = val;
                       break; 
                   }
        case 0x04:
                   writeIO32(addr, val);
                   break;
        
        case 0x05: {
            uint32_t off = (addr - 0x05000000) & 0x7FF;
            *reinterpret_cast<uint32_t*>(&pallete[off]) = val;
            break;
        }
        case 0x06: {
            uint32_t off = addr - 0x06000000;
            if (off < 0x100000)
                *reinterpret_cast<uint32_t*>(&vramA[off & 0xFFFFC]) = val;
            break;
        }
        case 0x07: {
            uint32_t off = (addr - 0x07000000) & 0x7FF;
            *reinterpret_cast<uint32_t*>(&oam[off]) = val;
            break;
        }

        default:
                   break;
    }
}

uint32_t Bus::readIO32(uint32_t addr){
    switch (addr) {
        case IME9:      return irq9.IME;
        case IE9:       return irq9.IE;
        case IF9:       return irq9.IF; 
        case DISPSTAT:  return 0;
        case VCOUNT:    return vcount;
        case KEYINPUT:  return keyinput;

        case 0x04000100:
            return timers9.timers[0].counter | (timers9.timers[0].control << 16);
        case 0x04000104:
            return timers9.timers[1].counter | (timers9.timers[1].control << 16);
        case 0x04000108:
            return timers9.timers[2].counter | (timers9.timers[2].control << 16);
        case 0x0400010C:
            return timers9.timers[3].counter | (timers9.timers[3].control << 16);

        default:        return 0;
    }
}


void Bus::writeIO32(uint32_t addr, uint32_t val){
    switch (addr) {
        case IME9: irq9.IME = val & 1; break;
        case IE9:  irq9.IE  = val;     break;
        case IF9:  irq9.acknowledgeIF(val); break;

        case 0x04000100: timers9.timers[0].reload  = val & 0xFFFF;
                         timers9.timers[0].control = val >> 16; break;
        case 0x04000104: timers9.timers[1].reload  = val & 0xFFFF;
                         timers9.timers[1].control = val >> 16; break;
        case 0x04000108: timers9.timers[2].reload  = val & 0xFFFF;
                         timers9.timers[2].control = val >> 16; break;
        case 0x0400010C: timers9.timers[3].reload  = val & 0xFFFF;
                         timers9.timers[3].control = val >> 16; break;

        default: break; 
    }
}
