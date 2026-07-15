#include "palette.h"

//RAMP generatod
// https:

uint32_t bswap32(uint32_t a) {
    return
        ((a & 0x000000FF) << 24) |
        ((a & 0x0000FF00) <<  8) |
        ((a & 0x00FF0000) >>  8) |
        ((a & 0xFF000000) >> 24);
}

/*
#FFFFCC
#FFF5B5
#FFEC9D
#FEE187
#FED470
#FEBF5A
#FEAB49
#FD9740
#FD7C37
#FC5B2E
#F43D25
#E6211E
#D41020
#C00225
#A10026
#800026
*/

std::vector<uint32_t> color_ramp = {
    bswap32(0xFFFFCCFF),
    bswap32(0xFFF5B5FF),
    bswap32(0xFFEC9DFF),
    bswap32(0xFEE187FF),
    bswap32(0xFED470FF),
    bswap32(0xFEBF5AFF),
    bswap32(0xFEAB49FF),
    bswap32(0xFD9740FF),
    bswap32(0xFD7C37FF),
    bswap32(0xFC5B2EFF),
    bswap32(0xF43D25FF),
    bswap32(0xE6211EFF),
    bswap32(0xD41020FF),
    bswap32(0xC00225FF),
    bswap32(0xA10026FF),
    bswap32(0x800026FF)
};
