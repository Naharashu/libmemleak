#include "src/libmemleak.h"

int main() {
    long* ptr = new long[1];
    for(int i=0;i<10000;i++) {
        delete[] ptr;
        ptr = new long[i];
    }
    delete[] ptr;
    return 0;
}