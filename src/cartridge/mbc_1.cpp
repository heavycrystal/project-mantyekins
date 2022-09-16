#include    "mbc_1.hpp"

#define     LOWER_ROM_BANK_UPDATE_      lower_rom_bank_data = full_cartridge_rom.get() + ((CARTRIDGE_ROM_ADDRESS_SIZE / 2) * current_lower_rom_bank)
#define     UPPER_ROM_BANK_UPDATE_      upper_rom_bank_data = full_cartridge_rom.get() + ((CARTRIDGE_ROM_ADDRESS_SIZE / 2) * current_upper_rom_bank)
#define     RAM_BANK_UPDATE_            ram_bank_data = full_cartridge_ram.get() + (CARTRIDGE_RAM_ADDRESS_SIZE * current_ram_bank) 

mbc1_cartridge_t::mbc1_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size): gb_cartridge_t(std::move(cartridge_rom), rom_size, ram_size) {
    rom_bank_count = 2 * (rom_size / CARTRIDGE_ROM_ADDRESS_SIZE);
    ram_bank_count = ram_size / CARTRIDGE_RAM_ADDRESS_SIZE;

    LOWER_ROM_BANK_UPDATE_;
    UPPER_ROM_BANK_UPDATE_;
    RAM_BANK_UPDATE_;
}

u8 mbc1_cartridge_t::cartridge_read(const u16 address) {
    if(address <= 0x3FFF) {
        return lower_rom_bank_data[address];
    }
    else if(address <= 0x7FFF) {
        return upper_rom_bank_data[address - 0x4000];
    }
    else if(BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        assert((address - 0xA000) < this->ram_size);
        if(!ram_enable) {
            INVALID_READ_;
        }
        return ram_bank_data[address - 0xA000];
    }
    /* invalid access, attempted to read SRAM while disabled or wrong address range */
    UNREACHABLE_;
}

void mbc1_cartridge_t::cartridge_write(const u16 address, const u8 value) {
    if(address <= 0x1FFF) {
        ram_enable = (BIT_CUT(value, 0, 4) == 0xA);
    }
    else if(address <= 0x3FFF) {
        current_upper_rom_bank = (BIT_CUT(current_upper_rom_bank, 5, 2) << 5) | (((BIT_CUT(value, 0, 5) == 0x00) ? 0x01 : BIT_CUT(value, 0, 5)) & (rom_bank_count - 1));
        UPPER_ROM_BANK_UPDATE_;
    }
    else if(address <= 0x5FFF) {
        if(rom_bank_count > 16) {
            current_upper_rom_bank = (BIT_CUT(value, 0, 2) << 5) | BIT_CUT(current_upper_rom_bank, 0, 5);
            UPPER_ROM_BANK_UPDATE_;
            if(banking_mode_select == 1) {
                current_lower_rom_bank = (BIT_CUT(value, 0, 2) << 5);
                LOWER_ROM_BANK_UPDATE_;
            }
        }
        else if((banking_mode_select == 1) && (ram_bank_count == 4)) {
            current_ram_bank = BIT_CUT(value, 0, 2);
            RAM_BANK_UPDATE_;
        }
    }
    else if(address <= 0x7FFF) {
        banking_mode_select = BIT_CUT(value, 0, 1);
        if(banking_mode_select == 0) {
            current_lower_rom_bank = 0x00;
            current_ram_bank = 0x00;
            LOWER_ROM_BANK_UPDATE_;
            RAM_BANK_UPDATE_;
        }
        else {
            if(rom_bank_count > 16) {
                current_lower_rom_bank = (BIT_CUT(current_upper_rom_bank, 5, 2) << 5);
                LOWER_ROM_BANK_UPDATE_;                    
            }
            else {
                current_ram_bank = BIT_CUT(current_upper_rom_bank, 5, 2);
                RAM_BANK_UPDATE_;                    
            }
        }
    }
    else if(BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        if(!ram_enable) {
            INVALID_WRITE_;
        }  
        assert((address - 0xA000) < this->ram_size);          
        ram_bank_data[address - 0xA000] = value;
    }
    else {
        UNREACHABLE_;
    }
}