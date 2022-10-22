#include    "mbc_3.hpp"
#include    "mbc_rtc.hpp"

#define     LOWER_ROM_BANK_UPDATE_      lower_rom_bank_data = full_cartridge_rom.get() + ((CARTRIDGE_ROM_ADDRESS_SIZE / 2) * current_lower_rom_bank)
#define     UPPER_ROM_BANK_UPDATE_      upper_rom_bank_data = full_cartridge_rom.get() + ((CARTRIDGE_ROM_ADDRESS_SIZE / 2) * current_upper_rom_bank)
#define     RAM_BANK_UPDATE_            ram_bank_data = full_cartridge_ram.get() + (CARTRIDGE_RAM_ADDRESS_SIZE * current_ram_rtc_bank) 

mbc3_cartridge_t::mbc3_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size, mbc_rtc_t* gb_rtc): gb_cartridge_t(std::move(cartridge_rom), rom_size, ram_size) {
    this->rom_bank_count = rom_size / CARTRIDGE_ROM_ADDRESS_SIZE;
    this->ram_bank_count = ram_size / CARTRIDGE_RAM_ADDRESS_SIZE;

    LOWER_ROM_BANK_UPDATE_;
    UPPER_ROM_BANK_UPDATE_;
    RAM_BANK_UPDATE_;
}

u8 mbc3_cartridge_t::cartridge_read(const u16 address) {
    if(address <= 0x3FFF) {
        return lower_rom_bank_data[address];
    }
    else if(address <= 0x7FFF) {
        return upper_rom_bank_data[address - 0x4000];
    }
    else if(BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        if(!ram_rtcreg_enable) {
            INVALID_READ_;
        }
        if(current_ram_rtc_bank < ram_bank_count) {
            assert((address - 0xA000) < this->ram_size);
            return ram_bank_data[address - 0xA000];
        }
        else if(BOUNDS_CHECK(current_ram_rtc_bank, 0x08, 0x0C)) {
            return gb_rtc_latched.data.as_raw[current_ram_rtc_bank - 0x08];
        }
        /* RAM-RTC banking register is guaranteed to be in sane range */
        UNREACHABLE_;
    }
    UNREACHABLE_;
}

void mbc3_cartridge_t::cartridge_write(const u16 address, const u8 value) {
    if(address <= 0x1FFF) {
        ram_rtcreg_enable = (BIT_CUT(value, 0, 4) == 0xA);
    }
    else if(address <= 0x3FFF) {
        current_upper_rom_bank = (BIT_CUT(value, 0, 7) == 0x00) ? 0x01 : BIT_CUT(value, 0, 7);
        UPPER_ROM_BANK_UPDATE_;
    }
    else if(address <= 0x5FFF) {
        if((value < ram_bank_count) || BOUNDS_CHECK(value, 0x08, 0x0C)) {
            current_ram_rtc_bank = value;
            if(value < ram_bank_count) {
                RAM_BANK_UPDATE_;
            }
        } 
        INVALID_WRITE_;
    }
    else if(address <= 0x7FFF) {
        if((rtc_latch_buffer == 0x00) && (value == 0x01)) {
            memcpy(&gb_rtc_latched.data.as_raw, gb_rtc->data.as_raw, sizeof(gb_rtc->data.as_raw));
        }
        rtc_latch_buffer = value;
    }
    else if(BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        if(!ram_rtcreg_enable) {
            INVALID_WRITE_;
        }
        if(current_ram_rtc_bank < ram_bank_count) {
            assert((address - 0xA000) < this->ram_size);
            ram_bank_data[address - 0xA000] = value;
        }
        else if(BOUNDS_CHECK(current_ram_rtc_bank, 0x08, 0x0C)) {
            /*  Pandocs claims halt flag should be set before writing to RTC registers */
            if(BIT_IS_SET(gb_rtc->data.as_name.day_high_status, 6)) {
                gb_rtc_latched.data.as_raw[current_ram_rtc_bank - 0x08] = value;
            }
            INVALID_WRITE_;
        }
        /* RAM-RTC banking register is guaranteed to be in sane range */
        UNREACHABLE_;
    }
    UNREACHABLE_;
}