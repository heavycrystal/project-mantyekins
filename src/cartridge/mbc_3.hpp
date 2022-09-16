#pragma     once

#include    "cartridge.hpp"
#include    "mbc_rtc.hpp"

class mbc3_cartridge_t: public gb_cartridge_t {
    /*  implemented primarily using: https://gbdev.io/pandocs/MBC3.html */  
    private:
    bool ram_rtcreg_enable = false;
    u8 rom_bank_count;
    u8 ram_bank_count;
    u8 current_lower_rom_bank = 0x00;
    u8 current_upper_rom_bank = 0x01;
    /*  this variable is used for RTC registers as well, but keeping name to reuse MBC1 macros, hehe */
    u8 current_ram_rtc_bank = 0x00;
    /* filling with non-zero value to prevent improper latches from sticking */
    u8 rtc_latch_buffer = 0xFF;

    u8* lower_rom_bank_data = nullptr;
    u8* upper_rom_bank_data = nullptr;
    u8* ram_bank_data = nullptr;

    mbc_rtc_t* gb_rtc;
    mbc_rtc_t gb_rtc_latched;

    public:
    mbc3_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size, mbc_rtc_t* gb_rtc);

    u8 cartridge_read(const u16 address) final;
    void cartridge_write(const u16 address, const u8 value) final;
};