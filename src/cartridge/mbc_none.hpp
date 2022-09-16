#pragma     once

#include    "cartridge.hpp"

class no_mbc_cartridge_t: public gb_cartridge_t {
    public:
    no_mbc_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size);

    u8 cartridge_read(const u16 address) final;
    void cartridge_write(const u16 address, const u8 value) final;
};