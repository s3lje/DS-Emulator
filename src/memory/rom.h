#ifndef __BUS_H
#define __BUS_H

#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct NDSHeader {
    char     gameTitle[12];
    char     gameCode[4];
    char     makerCode[2];
    uint8_t  unitCode;
    uint8_t  encryptionSeed;
    uint8_t  deviceCapacity;
    uint8_t  reserved[7];
    uint8_t  region;
    uint8_t  romVersion;
    uint8_t  autostart;
    uint8_t  unused; 

    uint32_t arm9RomOffset;
    uint32_t arm9EntryAddress;
    uint32_t arm9RamAddress;
    uint32_t arm9Size;

    uint32_t arm7RomOffset;
    uint32_t arm7EntryAddress;
    uint32_t arm7RamAddress;
    uint32_t arm7Size;
} __attribute__((packed));



#endif
