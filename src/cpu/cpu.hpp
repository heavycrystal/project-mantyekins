#pragma     once

#include    "../common/common.hpp"

#include    "../ioregs/ioregs.hpp"
#include    "../memory/memory.hpp"
#include    "../timer/timer.hpp"

class gb_cpu_t {
    private:
    u16_as_u8_t af;
    u16_as_u8_t bc;
    u16_as_u8_t de;
    u16_as_u8_t hl;
    u16_as_u8_t sp { .as_u16 = 0xFFFE };
    u16_as_u8_t pc { .as_u16 = 0x0100 };

    uint64_t cycle_count = 0;
    /* this is a scratch register used for various operations */
    u16_as_u8_t scratch;
    bool interrupt_master_enable = false;
    bool is_halted = false;
    enum : u8 {
        IME_UNSET,
        IME_SETTING1,
        IME_SETTING2,
    } ime_scheduled = IME_UNSET;

    public:
    bool is_stepping = false;
    const uint64_t& ro_cycle_count;

    gb_cpu_t();

    void handle_interrupts(gb_memory_t* gb_memory, gb_timer_t* gb_timer, gb_ioregs_t* gb_ioregs);
    void fetch_and_execute_instruction(gb_memory_t* gb_memory, gb_timer_t* gb_timer);
};

