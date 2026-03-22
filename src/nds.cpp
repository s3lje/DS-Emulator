#include "nds.h"
#include <iostream>

bool NDS::loadROM(const std::string& path){
    std::cout << "Loading ROM: " << path << std::endl;
    return true;
}

void NDS::run(){
    // Stub for now... will implement main loop later
    std::cout << "NDS::run() called...\n";
}
