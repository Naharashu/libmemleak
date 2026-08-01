#include "src/libmemleak.h"

int main() {
    long* ptr = new long[1];
    long* ptr2 = new long;
    long* ptr3 = new long[2];
    delete ptr3;
    return 0;
}