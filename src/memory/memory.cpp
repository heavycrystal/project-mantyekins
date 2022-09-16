#include    "memory.hpp"

gb_memory_t::gb_memory_t(char* cartridge_file_name, gb_ioregs_t* gb_ioregs, gb_vram_t* gb_vram, gb_oam_t* gb_oam) : gb_ioregs(gb_ioregs), gb_vram(gb_vram), gb_oam(gb_oam) {
    gb_cartridge = verify_and_load_cartridge(cartridge_file_name);
}

u8* gb_memory_t::memory_mapper(const u16 address) {
    if(BOUNDS_CHECK(address, 0x8000, 0x9FFF)) {
        if(!this->gb_vram->is_locked) {
            return &gb_vram->data.as_raw[address - 0x8000]; /* in video RAM */
        }
        /* invalid memory access, video RAM access while locked */
    }
    else if(BOUNDS_CHECK(address, 0xC000, 0xDFFF)) {
        return &work_ram[address - 0xC000]; /* in work RAM */
    }
    else if(BOUNDS_CHECK(address, 0xFE00, 0xFE9F)) {
        if(!this->gb_oam->is_locked) {
            return &gb_oam->data.as_raw[address - 0xFE00]; /* in OAM */
        }
        /* invalid memory access, OAM access while locked */        
    }
    else if(BOUNDS_CHECK(address, 0xFF80, 0xFFFE)) {
        return &high_ram[address - 0xFF80]; /* in high RAM */
    }
    else if(address == 0xFFFF) {
        return &gb_ioregs->as_name.ie;
    }
    UNREACHABLE_;
}

u8 gb_memory_t::read_memory(const u16 address) {
#if ENABLE_READ_BREAKPOINTS
    for(u16 loop_var : READ_BREAKPOINTS) {
        if(address == loop_var) {
            fprintf(stderr, "Read breakpoint reached at %04x, entering single stepping mode.\n", loop_var);
            is_stepping = true;
        }
    }
#endif
    if((address <= 0x7FFF) || BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        return gb_cartridge->cartridge_read(address);
    }
    else if(BOUNDS_CHECK(address, 0xFF00, 0xFF7F)) {
        return gb_ioregs->ioregs_read(address);
    }
    else {
        return *memory_mapper(address);
    }
}

void gb_memory_t::write_memory(const u16 address, const u8 value) {
#if ENABLE_WRITE_BREAKPOINTS
    for(u16 loop_var : WRITE_BREAKPOINTS) {
        if(address == loop_var) {
            fprintf(stderr, "Write breakpoint reached at %04x with value %02x, entering single stepping mode.\n", loop_var, value);
            is_stepping = true;
        }
    }
#endif        
    if((address <= 0x7FFF) || BOUNDS_CHECK(address, 0xA000, 0xBFFF)) {
        gb_cartridge->cartridge_write(address, value);
    }
    else if(BOUNDS_CHECK(address, 0xFF00, 0xFF7F)) {
        gb_ioregs->ioregs_write(address, value);
    }
    else {
        *memory_mapper(address) = value;
    }
}