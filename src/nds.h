#pragma once
#include <string>

class NDS {
    public:
        bool loadROM(const std::string& path);
        void run();
};
