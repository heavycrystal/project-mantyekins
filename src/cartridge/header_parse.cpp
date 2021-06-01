#include "cartridge.h"

void cartridge_header::populate_raw_struct_data(uint8_t* file_data)
{
    memcpy(header_data.entry_point, file_data + CARTRIDGE_ENTRY_POINT_START, SECTION_SIZE(CARTRIDGE_ENTRY_POINT));
    memcpy(header_data.nintendo_logo_data, file_data + CARTRIDGE_NINTENDO_LOGO_START, SECTION_SIZE(CARTRIDGE_NINTENDO_LOGO));    
    memcpy(header_data.cartridge_title_superset, file_data + CARTRIDGE_TITLE_SUPERSET_START, SECTION_SIZE(CARTRIDGE_TITLE_SUPERSET));
    memcpy(header_data.cartridge_manufacturer_code, file_data + CARTRIDGE_MANUFACTURER_CODE_START, SECTION_SIZE(CARTRIDGE_MANUFACTURER_CODE));
    memcpy(header_data.new_licensee_code, file_data + CARTRIDGE_NEW_LICENSEE_CODE_START, SECTION_SIZE(CARTRIDGE_NEW_LICENSEE_CODE));
    memcpy(header_data.global_checksum, file_data + CARTRIDGE_GLOBAL_CHECKSUM_START, SECTION_SIZE(CARTRIDGE_GLOBAL_CHECKSUM));

    header_data.cgb_flag = file_data[CARTRIDGE_CGB_FLAG];
    header_data.sgb_flag = file_data[CARTRIDGE_SGB_FLAG];
    header_data.cartridge_type_data = file_data[CARTRIDGE_TYPE];
    header_data.rom_size_data = file_data[CARTRIDGE_ROM_SIZE];
    header_data.ram_size_data = file_data[CARTRIDGE_RAM_SIZE];
    header_data.destination_code = file_data[CARTRIDGE_DESTINATION_CODE];
    header_data.old_licensee_code = file_data[CARTRIDGE_OLD_LICENSEE_CODE];
    header_data.mask_rom_version_number = file_data[CARTRIDGE_MASK_ROM_VERSION];
    header_data.header_checksum = file_data[CARTRIDGE_HEADER_CHECKSUM];
}

bool cartridge_header::nintendo_logo_check()
{
    for(int i = 0; i < SECTION_SIZE(CARTRIDGE_NINTENDO_LOGO); i++)
    {
        if(NINTENDO_LOGO[i] != (header_data.nintendo_logo_data)[i])
        {
            return false;
        }
    }

    return true;
}
bool cartridge_header::cartridge_type_parse()
{
    header_data.mbc_type = MBC_NONE;
    header_data.is_ram_present = false;
    header_data.is_battery_present = false;
    header_data.is_timer_present = false;

// skipping unimplemented cases 0x08 and 0x09, as well as unimplemented MBCs [for now?]
    switch(header_data.cartridge_type_data)
    {
        case 0x00:
            break;
        case 0x01: 
            header_data.mbc_type = MBC_1;
            break;
        case 0x02:
            header_data.mbc_type = MBC_1;
            header_data.is_ram_present = true;
            break;
        case 0x03:
            header_data.mbc_type = MBC_1;
            header_data.is_ram_present = true;
            header_data.is_battery_present = true;
            break;
        case 0x05:
            header_data.mbc_type = MBC_2;
            break; 
        case 0x06:
            header_data.mbc_type = MBC_2;
            header_data.is_battery_present = true;
            break;
        case 0x0F:
            header_data.mbc_type = MBC_3;
            header_data.is_timer_present = true;
            header_data.is_battery_present = true;
            break;
        case 0x10:
            header_data.mbc_type = MBC_3;
            header_data.is_timer_present = true;
            header_data.is_ram_present = true;
            header_data.is_battery_present = true;
            break;
        case 0x11:
            header_data.mbc_type = MBC_3;
            break;
        case 0x12:
            header_data.mbc_type = MBC_3;
            header_data.is_ram_present = true;
            break;
        case 0x13:
            header_data.mbc_type = MBC_3;
            header_data.is_ram_present = true;
            header_data.is_battery_present = true;
            break;
        case 0x19:
            header_data.mbc_type = MBC_5;   
            break;
        case 0x1A:
            header_data.mbc_type = MBC_5;
            header_data.is_ram_present = true;
            break;
        case 0x1B:
            header_data.mbc_type = MBC_5;
            header_data.is_ram_present = true;
            header_data.is_battery_present = true;
            break;  
        default:
            header_data.mbc_type = UNSUPPORTED_MBC;
            return false;                          
    }

    return true;
}
bool cartridge_header::rom_size_parse()
{
    if(header_data.rom_size_data < 0x09)
    {
        header_data.rom_size = (32 * KB) << (header_data.rom_size_data);
        return true;
    }
    return false;
}
bool cartridge_header::ram_size_parse()
{
    switch(header_data.ram_size_data)
    {
        case 0x00:
            header_data.ram_size = 0;
            break;
        case 0x02:
            header_data.ram_size - 8 * KB;
            break;
        case 0x03:
            header_data.ram_size = 32 * KB;
            break;
        case 0x04:
            header_data.ram_size = 128 * KB;
            break;
        case 0x05:
            header_data.ram_size = 64 * KB;
            break;
        default:
            return false;                    
    }

    return true;
}
bool cartridge_header::header_checksum_verify(uint8_t* file_data)
{
    uint8_t checksum = 0;
    
    for(uint32_t i = 0x0134; i <= 0x014C; i++)
    {
        checksum = checksum - file_data[i] - 1;
    }

    return (checksum == header_data.header_checksum);
}

void cartridge_header::derive_header_data(uint8_t* file_data)
{
    header_data.is_header_valid = true;

    header_data.nintendo_logo_match = nintendo_logo_check();
    header_data.is_bootable = nintendo_logo_check();

    header_data.valid_cartridge_type = cartridge_type_parse();
    header_data.is_header_valid = (header_data.is_header_valid & cartridge_type_parse());
    header_data.valid_rom_size = rom_size_parse();
    header_data.is_header_valid = (header_data.is_header_valid & rom_size_parse());

    if(header_data.is_ram_present)
    {
        header_data.valid_ram_size = ram_size_parse();
        header_data.is_header_valid = (header_data.is_header_valid & ram_size_parse());
    }

    header_data.header_checksum_clear = header_checksum_verify(file_data);
    header_data.is_bootable = header_checksum_verify(file_data);

    header_data.global_checksum_clear = true;
}

bool cartridge_header::was_parse_successful()
{
    return header_data.is_header_valid;
}
bool cartridge_header::is_cartridge_bootable()
{
    return header_data.is_bootable;
} 
bool cartridge_header::is_cartridge_official()
{
    return header_data.is_official;
}
bool cartridge_header::is_ram_present()
{
    return header_data.is_ram_present;
}
bool cartridge_header::is_battery_present()
{
    return header_data.is_battery_present;
}
bool cartridge_header::is_timer_present()
{
    return header_data.is_timer_present;
}

cartridge_header::mbc_type_enum cartridge_header::get_mbc_type()
{
    return header_data.mbc_type;
} 
uint32_t cartridge_header::get_rom_size()
{
    return header_data.rom_size;
} 
uint32_t cartridge_header::get_ram_size()
{
    return header_data.ram_size;
} 

