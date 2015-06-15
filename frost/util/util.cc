#include "util.h"

namespace frost {
    int atoi_positive(char* str, size_t len){
        char* end = str + len;
        int n = 0;
        while (str != end) {
            n = n * 10 + (*str++ - '0');
        }
        return n;
    }
}