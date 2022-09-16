#include    "mbc_none.hpp"
#include <memory>

no_mbc_cartridge_t::no_mbc_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size): gb_cartridge_t(std::move(cartridge_rom), rom_size, ram_size) {}

u8 no_mbc_cartridge_t::cartridge_read(u16 address) {
    if(address <= 0x7FFF) {
        return full_cartridge_rom[address];
    }
    else if(BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        assert((address - 0xA000) < this->ram_size);
        return full_cartridge_ram[address - 0xA000];
    }
    UNREACHABLE_;
}

void no_mbc_cartridge_t::cartridge_write(u16 address, u8 value) {
    if(BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        assert((address - 0xA000) < this->ram_size);
        full_cartridge_ram[address - 0xA000] = value;
    }
    UNREACHABLE_;
}