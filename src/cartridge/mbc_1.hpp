#pragma     once

#include    "cartridge.hpp"

class mbc1_cartridge_t: public gb_cartridge_t {
    /*  implemented primarily using: https://gbdev.io/pandocs/MBC1.html
        banking modes also understood via: https://gekkio.fi/files/gb-docs/gbctr.pdf */    
    private:
    bool ram_enable = false;
    u8 rom_bank_count;
    u8 ram_bank_count;
    u8 current_lower_rom_bank = 0x00;
    u8 current_upper_rom_bank = 0x01; /* cannot be set to 0x00, enforced by MBC1 */ 
    u8 current_ram_bank = 0x00;
    u8 banking_mode_select = 0x00;

    u8* lower_rom_bank_data = nullptr;
    u8* upper_rom_bank_data = nullptr;
    u8* ram_bank_data = nullptr;

    public:
    mbc1_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size);

    u8 cartridge_read(const u16 address) final;
    void cartridge_write(const u16 address, const u8 value) final;
};