#include    "cartridge.hpp"

gb_cartridge_t::gb_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size) : ram_size(ram_size) {
    this->full_cartridge_rom = std::move(cartridge_rom);
    if(ram_size > 0) {
        this->full_cartridge_ram = std::unique_ptr<u8[]>(new u8[ram_size]);
    }
}