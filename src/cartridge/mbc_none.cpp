#include    "cartridge.h"

class mbc_none: public mbc_subsystem
{
    public:

    mbc_none() = delete;
    mbc_none(u8* cartridge_data, u8* cartridge_sram): mbc_subsystem(cartridge_data, cartridge_sram)
    {

    }

    u8 read_byte(u16 address)
    {
        return mbc_subsystem_internal_common.cartridge_data[address];
    }
    void write_byte(u16 address, u8 value)
    {
        mbc_subsystem_internal_common.cartridge_data[address] = value;
        return;
    }
};