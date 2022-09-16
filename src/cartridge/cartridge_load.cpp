#include    "cartridge_load.hpp"
#include "cartridge.hpp"
#include    <memory>

/*  Open a given file, check if it is a valid cartridge ROM size as we expect, and then load it to memory.
    Checks if opened file in fact a valid GB cartridge and also one that we do support, via rudimentary header checks.
    If so, setup the appropriate MBC and initialize gb_cartridge object. */
std::unique_ptr<gb_cartridge_t> verify_and_load_cartridge(char* cartridge_file_name) {
    FILE* cartridge_file = fopen(cartridge_file_name, "rb");
    if(cartridge_file == nullptr) {
        ERR_EXIT(FILE_OPEN_ERROR);
    }

    /* Reading cartridge header first, processing it and then loading the entire thing into memory */
    u8 cartridge_header[CARTRIDGE_HEADER_SIZE];
    /*  Aborting if the file opened does not even contain enough data for the header. */
    if(fread(cartridge_header, 1, CARTRIDGE_HEADER_SIZE, cartridge_file) != CARTRIDGE_HEADER_SIZE) {
        ERR_EXIT(ferror(cartridge_file) ? FILE_READ_ERROR : ROM_SIZE_INVALID_ERROR);
    }

    /* Performing header checksum calculation */
    u8 checksum = 0;
    for(int loop_var = 0x0134; loop_var <= 0x014C; loop_var++) {
        checksum = checksum - cartridge_header[loop_var] - 1;
    }
    if(cartridge_header[0x014D] != checksum) {
        ERR_EXIT(ROM_CHECKSUM_ERROR);
    }

    /*  Reading entire ROM into memory based on what the header says the ROM size is.
        Ensure this is deleted in the destructor of every class it is handed off to */
    if(cartridge_header[0x0148] > 0x08) {
        /* invalid cartridge ROM size, abort! */
    }
    size_t rom_size = CARTRIDGE_ROM_ADDRESS_SIZE << cartridge_header[0x0148];
    std::unique_ptr<u8[]> cartridge_rom(new u8[rom_size]);
    memcpy(cartridge_rom.get(), cartridge_header, CARTRIDGE_HEADER_SIZE);
    if(fread(cartridge_rom.get() + CARTRIDGE_HEADER_SIZE, 1, rom_size - CARTRIDGE_HEADER_SIZE + 1, cartridge_file) != (rom_size - CARTRIDGE_HEADER_SIZE)) {
        cartridge_rom.reset();
        ERR_EXIT(ferror(cartridge_file) ? FILE_READ_ERROR : ROM_SIZE_INVALID_ERROR);
    }
    if(fclose(cartridge_file)) {
        cartridge_rom.reset();
        ERR_EXIT(FILE_CLOSE_ERROR);
    }

    size_t ram_size = CARTRIDGE_RAM_ADDRESS_SIZE;
    switch(cartridge_header[0x0149]) {
        case 0x00:
            ram_size *= 0;
            break;
        case 0x02:
            break;
        case 0x03:
            ram_size *= 4;
            break;
        default:
            /* invalid cartridge RAM size, abort! */  
            break; 
    } 
    
    switch(cartridge_header[0x0147]) {
        case 0x00:
        case 0x08:
        case 0x09:
            return std::make_unique<no_mbc_cartridge_t>(std::move(cartridge_rom), rom_size, ram_size);
        case 0x01:
        case 0x02:
        case 0x03:
            return std::make_unique<mbc1_cartridge_t>(std::move(cartridge_rom), rom_size, ram_size);
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            /* MBC3 */
            break;
        default:
            /* unsupported MBC, abort! */
            break;
    }
    UNREACHABLE_;
}