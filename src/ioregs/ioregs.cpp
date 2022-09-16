#include    "ioregs.hpp"

gb_ioregs_t::gb_ioregs_t(): as_name() {}

u8 gb_ioregs_t::ioregs_read(const u16 address) {
    return this->as_raw[address - 0xFF00];
}

void gb_ioregs_t::ioregs_write(const u16 address, const u8 value) {
    switch(address) {
        case 0xFF04: /* reset DIV to 0 on any write */
            this->as_name._unmapped_FF03 = 0x00;
            this->as_name.div = 0x00;
            break;
        default:
            this->as_raw[address - 0xFF00] = value;
    }
}