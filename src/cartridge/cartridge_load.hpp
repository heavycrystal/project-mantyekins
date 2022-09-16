#pragma     once

#include    "../common/common.hpp"
#include    "../common/error_strings.hpp"

#include    "cartridge.hpp"
#include    "mbc_none.hpp"
#include    "mbc_1.hpp"
#include    "mbc_3.hpp"


/*  The header is actually 80 bytes, but it starts from 0x100 instead of the beginning of the ROM
    So we are forced to read those 256 bytes as well or perform some annoying reads */
#define     CARTRIDGE_HEADER_SIZE       (256 + 80)

std::unique_ptr<gb_cartridge_t> verify_and_load_cartridge(char* cartridge_file_name);