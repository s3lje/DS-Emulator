#include "nds.h"

int main(int argc, char* argv[]){
    NDS nds;
    nds.loadROM(argv[1]);
    nds.run();
}
