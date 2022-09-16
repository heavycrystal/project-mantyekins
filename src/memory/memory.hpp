#pragma     once

#include    "../common/common.hpp"
#include    "../common/error_strings.hpp"

#include    "../cartridge/cartridge.hpp"
#include    "../cartridge/cartridge_load.hpp"
#include    "../ioregs/ioregs.hpp"
#include    "../ppu/ppu.hpp"
#include <memory>

#define     WORK_RAM_ADDRESS_SIZE       8192
#define     HIGH_RAM_ADDRESS_SIZE       127

class gb_memory_t {
    private:
    std::unique_ptr<gb_cartridge_t> gb_cartridge;
    u8              work_ram[WORK_RAM_ADDRESS_SIZE] = { };
    gb_vram_t*      gb_vram;
    gb_ioregs_t*    gb_ioregs;
    gb_oam_t*       gb_oam;
    u8              high_ram[HIGH_RAM_ADDRESS_SIZE] = { };

    /* deleting copy constructor and copy assignment operator */
    gb_memory_t& operator = (const gb_memory_t&) = delete;
    gb_memory_t(const gb_memory_t&) = delete;

    public:
    gb_memory_t(char* cartridge_file_name, gb_ioregs_t* gb_ioregs, gb_vram_t* gb_vram, gb_oam_t* gb_oam);

    /*  This helper function directs many memory accesses by the CPU to the appropriate area, except the areas that need special handling.
        It also enforces the OAM/video RAM locking by the video subsystem/PPU. Unlike most emulators, even reads to locked RAM are seen as illegal.
        It aborts the emulator if any invalid memory accesses are made, otherwise returns a pointer to the requested byte.
        Due to how the Game Boy is implemented, the region [E000 - FDFF] actually redirects to work RAM at [C000 - DDFF].
        It is known unofficially as echo RAM. Nintendo officially does not allow games to use this memory area, so it is treated as an illegal memory acess */
    public:
    u8* memory_mapper(const u16 address);
    u8 read_memory(const u16 address);
    void write_memory(const u16 address, const u8 value);
};