#include    "cartridge.h"

class mbc_1: public mbc_subsystem
{
#ifndef DEBUG    
    private:
#else
    public:
#endif 

    struct mbc_1_internal_struct
    {
        u8      current_lower_rom_bank;
        u8      current_upper_rom_bank_lower_bits;
        u8      current_upper_rom_bank_upper_bits;
        u8      current_ram_bank;

        bool    is_large_rom_cartridge;
        bool    ram_enable;
        bool    banking_mode;    
    }   mbc_1_internal;

    public:

    mbc_1() = delete;
    mbc_1(u8* cartridge_data, u8* cartridge_sram, cartridge_header header): mbc_subsystem(cartridge_data, cartridge_sram)
    {
        mbc_1_internal.current_lower_rom_bank = 0x00;
        mbc_1_internal.current_upper_rom_bank_lower_bits = 0x01;
        mbc_1_internal.current_upper_rom_bank_upper_bits = 0x00;
        mbc_1_internal.current_ram_bank = 0x00;

        mbc_1_internal.is_large_rom_cartridge = (header.get_rom_size() >= 1 * MB);
        mbc_1_internal.ram_enable = false;
        mbc_1_internal.banking_mode = false;
    }

    u8 read_byte(u16 address)
    {
        if(address <= 0x3FFF)
        {
            return mbc_subsystem_internal_common.cartridge_data[(mbc_1_internal.current_lower_rom_bank * 16 * KB) + address];
        }
        else if(address <= 0x7FFF)
        {
            return mbc_subsystem_internal_common.cartridge_data[(((mbc_1_internal.current_upper_rom_bank_upper_bits << 5) + (mbc_1_internal.current_upper_rom_bank_lower_bits)) * 16 * KB) + address - 0x4000];
        }
        else if((address >= 0xA000) && (address <= 0xBFFF))
        {
            return mbc_subsystem_internal_common.cartridge_sram[(mbc_1_internal.current_ram_bank * 8 * KB) + address - 0xA000];
        }

//signal handler
    }

    void write_byte(u16 address, u8 value)
    {
        if(address <= 0x1FFF)
        {
            mbc_1_internal.ram_enable = ((value & 0x0F) == 0x0A);
        }
        else if(address <= 0x3FFF)
        {
            mbc_1_internal.current_upper_rom_bank_lower_bits = (value & 0x1F);
        }
        else if(address <= 0x5FFF)
        {
            if(mbc_1_internal.is_large_rom_cartridge)
            {
                mbc_1_internal.current_upper_rom_bank_upper_bits = (value & 0x03);
            }
            else if(mbc_1_internal.banking_mode)
            {
                mbc_1_internal.current_ram_bank = (value & 0x03);
            }
        }
        else if(address <= 0x7FFF)
        {
            mbc_1_internal.banking_mode = value;
            if(mbc_1_internal.banking_mode)
            {
                if(mbc_1_internal.is_large_rom_cartridge)
                {
                    mbc_1_internal.current_lower_rom_bank = (mbc_1_internal.current_upper_rom_bank_upper_bits << 5);
                }
                else
                {
                    mbc_1_internal.current_ram_bank = mbc_1_internal.current_upper_rom_bank_upper_bits;
                }
            }
            else
            {
                if(mbc_1_internal.is_large_rom_cartridge)
                {
                    mbc_1_internal.current_lower_rom_bank = 0x00;
                }
                else
                {
                    mbc_1_internal.current_ram_bank = 0x00;
                }
            }
        }
        else if((address >= 0xA000) && (address <= 0xBFFF))
        {
            mbc_subsystem_internal_common.cartridge_sram[(mbc_1_internal.current_ram_bank * 8 * KB) + address - 0xA000] = value; 
        }        
    }     
};