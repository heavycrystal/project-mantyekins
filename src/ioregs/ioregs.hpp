#pragma     once

#include    "../common/common.hpp"

/*  Extra register for interrupt enable [IE], which is located at 0xFFFF */
#define     IO_REGISTERS_ADDRESS_SIZE   (128 + 1)

/*  To improve code readability, the IO registers memory array is in a union with a struct that contains the actual names of these 
    IO registers, the names, default values and information. Unmapped registers are read as 0xFF and writes are ignored.
    This is a major component of the emulator's state, and mutable pointers are held by multiple components */
union gb_ioregs_t {
    u8 as_raw[IO_REGISTERS_ADDRESS_SIZE];
    struct io_registers_as_name_t {
        u8 p1 = 0xCF; /* joypad input */
        u8 sb = 0x00; /* serial transfer data */
        u8 sc = 0x7E; /* serial transfer control */
        u8 _unmapped_FF03 = 0xCC; /* repurposed for lower byte of timer internal counter */
        u8 div = 0xAB; /* timer divider */
        u8 tima = 0x00; /* timer counter */
        u8 tma = 0x00; /* timer modulo */
        u8 tac = 0xF8; /* timer control */
        u8 _unmapped_FF08_FF0E[7];
        u8 iflag = 0xE1; /* interrupt flag */
        u8 nr10 = 0x80; /* channel 1 sweep register */
        u8 nr11 = 0xBF; /* channel 1 sound length/wave pattern duty */
        u8 nr12 = 0xF3; /* channel 1 volume envelope */
        u8 nr13 = 0xFF; /* channel 1 frequency low byte */
        u8 nr14 = 0xBF; /* channel 1 frequency high bits + control */
        u8 _unmapped_FF15 = 0xFF;
        u8 nr21 = 0x3F; /* channel 2 sound length/wave pattern duty */
        u8 nr22 = 0x00; /* channel 2 volume envelope */
        u8 nr23 = 0xFF; /* channel 2 frequency low byte */
        u8 nr24 = 0xBF; /* channel 2 frequency high byte */
        u8 nr30 = 0x7F; /* channel 3 sound on/off */
        u8 nr31 = 0xFF; /* channel 3 sound length */
        u8 nr32 = 0x9F; /* channel 3 select output level */
        u8 nr33 = 0xFF; /* channel 3 frequency low byte */
        u8 nr34 = 0xBF; /* channel 3 frequency high bits  + control */
        u8 _unmapped_FF1F = 0xFF;
        u8 nr41 = 0xFF; /* channel 4 sound length */
        u8 nr42 = 0x00; /* channel 4 volume envelope */
        u8 nr43 = 0x00; /* channel 4 polynomial counter */
        u8 nr44 = 0xBF; /* channel 4 counter/consecutive */
        u8 nr50 = 0x77; /* master control/volume */
        u8 nr51 = 0xF3; /* selection of sound output terminal */
        u8 nr52 = 0xF1; /* sound on/off */
        u8 _unmapped_FF27_FF3F[25];
        u8 lcdc = 0x91; /* LCD control */
        u8 stat = 0x85; /* LCD status */
        u8 scy = 0x00; /* scroll Y */
        u8 scx = 0x00; /* scroll X */
        u8 ly = 0x00; /* LCD Y coordinate */
        u8 lyc = 0x00; /* LY compare */
        u8 dma = 0xFF; /* DMA transfer and start address */
        u8 bgp = 0xFC; /* BG palette data */
        u8 obp0 = 0xFF; /* OBJ palette 0 data, uninitialized at startup */
        u8 obp1 = 0xFF; /* OBJ palette 1 data, uninitialized at startup */
        u8 wy = 0x00; /* window Y position */
        u8 wx = 0x00; /* window X position */
        u8 _unmapped_FF4C_FF7F[52];
        u8 ie = 0x00; /* interrupt enable */
    } as_name;

    gb_ioregs_t(); 

    u8 ioregs_read(const uint16_t address);
    void ioregs_write(const uint16_t address, const u8 value);
};