#include    "timer.hpp"

gb_timer_t::gb_timer_t(gb_ioregs_t* gb_ioregs) : gb_ioregs(gb_ioregs) {
    internal_div = reinterpret_cast<u16_as_u8_t*>(&gb_ioregs->as_name._unmapped_FF03);
}

#if     ENABLE_DEBUG_PRINTF
void gb_timer_t::debug_printf() {
    printf("TIMA = %02x, TMA = %02x, TAC = %02x\n", _TIMA, _TMA, gb_ioregs->as_name.tac);
}
#endif

void gb_timer_t::div_increment() {
    /*  DIV is always counting, irrespective of TAC enable. */
    internal_div->as_u16 += 4;
    if(TIMER_ENABLE(gb_ioregs->as_name.tac) && ((internal_div->as_u16 % TIMA_DIV_FACTOR(gb_ioregs->as_name.tac)) == 0)) {
        tima_increment();
    }
}

void gb_timer_t::tima_increment() {
    gb_ioregs->as_name.tima++;
    if(gb_ioregs->as_name.tima == 0x00) {
        gb_ioregs->as_name.tima = gb_ioregs->as_name.tma;
        /*  TODO: replace magic number of interrupt */
        SET_BIT(gb_ioregs->as_name.iflag, 2);
    }
}