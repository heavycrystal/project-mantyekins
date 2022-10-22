#pragma     once

#include    "../common/common.hpp"

struct client_screen_t {
    struct {
        u8 red;
        u8 green;
        u8 blue;
    } pixels[144][160]; /* X and Y are reversed to better represent scanlie based rendering. */
};