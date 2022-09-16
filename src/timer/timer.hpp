#pragma     once

#include    "../common/common.hpp"
#include    "../ioregs/ioregs.hpp"

/* Timer macros */
#define     TIMER_ENABLE(input)         BIT_CUT(input, 2, 1)
#define     TIMA_DIV_FACTOR(input)      TERNARY_4_WAY(BIT_CUT(input, 0, 2), 1024, 16, 64, 256)          
#define     TIMA_DIV_FACTOR_BITS(input) TERNARY_4_WAY(BIT_CUT(input, 0, 2), 10, 4, 6, 8)

/*  https://gbdev.gg8.se/wiki/articles/Timer_Obscure_Behaviour and The Cycle Accurate GameBoy docs both document a lot 
    of weird and obscure timer behaviour. It is very interesting, but I don't see any point of emulating it at the moment. */
class gb_timer_t {
    private:
    gb_ioregs_t* gb_ioregs;
    /*  https://raw.githubusercontent.com/geaz/emu-gameboy/master/docs/The%20Cycle-Accurate%20Game%20Boy%20Docs.pdf 
        This document suggests that DIV is actually a 16-bit system register, with the upper 8 bits mapped into memory.
        The value for the lower byte only has one source [cycle-accurate gameboy docs], cannot be certain. */
    u16_as_u8_t* internal_div = nullptr; /* mapped into IO registers at FF03-FF04 */

    /* deleting copy constructor and copy assignment operator */
    gb_timer_t& operator = (const gb_timer_t&) = delete;
    gb_timer_t(const gb_timer_t&) = delete;

    public:
    gb_timer_t(gb_ioregs_t* gb_io_registers);

    void div_increment();

#if     ENABLE_DEBUG_PRINTF
    void debug_printf();
#endif

    private:
    void tima_increment();
};