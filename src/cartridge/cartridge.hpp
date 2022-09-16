#pragma     once

#include    "../common/common.hpp"

#define     CARTRIDGE_ROM_ADDRESS_SIZE  32768
#define     CARTRIDGE_RAM_ADDRESS_SIZE  8192

/*  This is an abstract class, an object cannot be constructed. */
class gb_cartridge_t {
    protected:
    std::unique_ptr<u8[]> full_cartridge_rom;
    std::unique_ptr<u8[]> full_cartridge_ram;
    size_t ram_size = 0;

    /* deleting copy constructor and copy assignment operator */
    gb_cartridge_t& operator = (const gb_cartridge_t&) = delete;
    gb_cartridge_t(const gb_cartridge_t&) = delete;

    public:
    gb_cartridge_t(std::unique_ptr<u8[]> cartridge_rom, size_t rom_size, size_t ram_size);

    // pure virtual methods
    virtual u8 cartridge_read(u16 address) = 0;
    virtual void cartridge_write(u16 address, u8 value) = 0;
};