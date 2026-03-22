#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "memory/rom.h"
#include "memory/bus.h"

class NDS {
    public:
        bool loadROM(const std::string& path);
        void run();

    private:
        NDSHeader header {};
        Bus bus;
};
