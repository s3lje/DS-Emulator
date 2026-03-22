#include <iostream>
#include "nds.h"

int main(int argc, char* argv[]){
    if (argc < 2){
        std::cerr << "Usage: " << argv[0] << " <rom.nds>\n";
        return EXIT_FAILURE;
    }

    NDS nds;
    if (!nds.loadROM(argv[1])){
        std::cerr << "Failed to load ROM...\n";
        return EXIT_FAILURE;
    }

    nds.run();

    return 0;
}
