#pragma     once

#include    <cstdio>
#include    <cstring>
#include    <cstdint>
#include    <cstdlib>
#include    <cassert>

#include    <memory>
#include    <algorithm>

typedef     uint8_t     u8;
typedef     uint16_t    u16;
typedef     uint32_t    u32;
typedef     uint64_t    u64;

/*  Technically the GameBoy CPU runs at 4 times this speed, but since every instruction consumes a multiple of 4 cycles
    we choose to work with the smaller cycle count, other peripherals are adjusted to this coarser clocking.
    Each tick of the 4x clock speed is referred to as a T-cycle, and for the 1x clock speed is an M-cycle */
#define     CYCLES_PER_SECOND           1048576

/*  common bit manipulation macros, used throughout the project */
#define     SET_BIT(input, bit)         (input |= (1 << (bit)))
#define     CLR_BIT(input, bit)         (input &= ~(1 << (bit)))
/*  Lobs off cut bits from input, and returns the count lowermost bits. 
    Generating bitmasks like this is not ideal, but hoping for constant folding to optimize it away. */
#define     BIT_CUT(input, cut, count)  (((input) >> (cut)) & ((1 << (count)) - 1))
#define     GET_BIT(input, bit)         BIT_CUT(input, bit, 1)
#define     BIT_IS_SET(input, bit)      (GET_BIT(input, bit) == 1)
#define     BIT_IS_CLR(input, bit)      (GET_BIT(input, bit) == 0)

#define     TRUNC_ADD(input1, input2)   ((input1 + input2) & 0xFF)

#define     TERNARY_4_WAY(cond, output_0, output_1, output_2, output_3) ((cond >= 2) ? ((cond == 2) ? output_2 : output_3) : ((cond == 0) ? output_0 : output_1))
#define     BOUNDS_CHECK(input, lower, upper)   ((input >= lower) && (input <= upper))

#define     UNREACHABLE_    assert(false)
#define     INVALID_READ_   return 0xFF
#define     INVALID_WRITE_  return
#define     ERR_EXIT(error) fprintf(stderr, error); \
                            exit(1)

/*  A very janky union, just for the sake of efficiency. 
    Should break under big-endian systems, and not sure how it interacts with struct padding. */
union u16_as_u8_t {
    u16 as_u16;
    struct {
        u8 lower;
        u8 upper;
    } as_u8;
};