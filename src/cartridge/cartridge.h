#include    <cstring>

#include    "../typedefs.h"

const u8 NINTENDO_LOGO[] = 
{   0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E  };


#define     CARTRIDGE_ENTRY_POINT_START         0x0100
#define     CARTRIDGE_ENTRY_POINT_END           0x0103
#define     CARTRIDGE_NINTENDO_LOGO_START       0x0104
#define     CARTRIDGE_NINTENDO_LOGO_END_cgb     0x011B
#define     CARTRIDGE_NINTENDO_LOGO_END         0x0133
#define     CARTRIDGE_TITLE_SUPERSET_START      0x0134
#define     CARTRIDGE_TITLE_SUPERSET_END        0x0143
#define     CARTRIDGE_MANUFACTURER_CODE_START   0x013F
#define     CARTRIDGE_MANUFACTURER_CODE_END     0x0142
#define     CARTRIDGE_CGB_FLAG                  0x0143
#define     CARTRIDGE_NEW_LICENSEE_CODE_START   0x0144
#define     CARTRIDGE_NEW_LICENSEE_CODE_END     0x0145   
#define     CARTRIDGE_SGB_FLAG                  0x0146 
#define     CARTRIDGE_TYPE                      0x0147
#define     CARTRIDGE_ROM_SIZE                  0x0148
#define     CARTRIDGE_RAM_SIZE                  0x0149
#define     CARTRIDGE_DESTINATION_CODE          0x014A
#define     CARTRIDGE_OLD_LICENSEE_CODE         0x014B
#define     CARTRIDGE_MASK_ROM_VERSION          0x014C
#define     CARTRIDGE_HEADER_CHECKSUM           0x014D
#define     CARTRIDGE_GLOBAL_CHECKSUM_START     0x014E
#define     CARTRIDGE_GLOBAL_CHECKSUM_END       0x014F

#define     SECTION_SIZE(section)               (section ## _END - section ## _START)
#define     KB                                  1024
#define     MB                                  1024 * KB

enum        mbc_type_enum: u8 { MBC_NONE, MBC_1, MBC_2, MBC_3, MBC_5, UNSUPPORTED_MBC };

class cartridge_header
{

#ifndef DEBUG    
    private:
#else
    public:
#endif

    struct cartridge_header_internal_struct
    {
        bool            is_header_valid;

        u8              entry_point[SECTION_SIZE(CARTRIDGE_ENTRY_POINT)];
        u8              nintendo_logo_data[SECTION_SIZE(CARTRIDGE_NINTENDO_LOGO)];
        u8              cartridge_title_superset[SECTION_SIZE(CARTRIDGE_TITLE_SUPERSET)];
        u8              cartridge_manufacturer_code[SECTION_SIZE(CARTRIDGE_MANUFACTURER_CODE)];
        u8              cgb_flag;
        u8              new_licensee_code[SECTION_SIZE(CARTRIDGE_NEW_LICENSEE_CODE)];
        u8              sgb_flag;
        u8              cartridge_type_data;
        u8              rom_size_data;
        u8              ram_size_data;
        u8              destination_code;
        u8              old_licensee_code;
        u8              mask_rom_version_number;
        u8              header_checksum;
        u8              global_checksum[SECTION_SIZE(CARTRIDGE_GLOBAL_CHECKSUM)];

        bool            nintendo_logo_match;
        bool            valid_cartridge_type;
        bool            valid_rom_size;
        bool            valid_ram_size;
        bool            header_checksum_clear;
        bool            global_checksum_clear;

        bool            is_bootable;
        bool            is_official;        
        
        bool            is_ram_present;
        bool            is_battery_present;
        bool            is_timer_present;

        mbc_type_enum   mbc_type;
        uint32_t        rom_size;
        uint32_t        ram_size;
    } header_data;

    void    populate_raw_struct_data(u8* cartridge_data);

    bool    nintendo_logo_check();
    bool    cartridge_type_parse();
    bool    rom_size_parse();
    bool    ram_size_parse();
    bool    header_checksum_verify(u8* cartridge_data);

    void    derive_header_data(u8* cartridge_data);

    public:

    cartridge_header() = delete;
    
    cartridge_header(u8* cartridge_data)
    {
        populate_raw_struct_data(cartridge_data);
        derive_header_data(cartridge_data);
    }

    bool            was_parse_successful();
    bool            is_cartridge_bootable();
    bool            is_cartridge_official();
    bool            is_ram_present();
    bool            is_battery_present();
    bool            is_timer_present();

    mbc_type_enum   get_mbc_type(); 
    uint32_t        get_rom_size();
    uint32_t        get_ram_size();                       
};

class mbc_subsystem
{
#ifndef DEBUG    
    protected:
#else
    public:
#endif    

    struct mbc_subsystem_internal_struct_common
    {
        u8*         cartridge_data;
        u8*         cartridge_sram;
    }  mbc_subsystem_internal_common;   

    public:

    mbc_subsystem() = delete;
    
    mbc_subsystem(u8* cartridge_data, u8* cartridge_sram)
    {
        mbc_subsystem_internal_common.cartridge_data = cartridge_data;
    }

    virtual u8 read_byte(u16 address);
    virtual void write_byte(u16 address, u8 value);    
};

class cartridge
{
#ifndef DEBUG    
    private:
#else
    public:
#endif

    struct cartridge_internal_struct
    {
        u8*                 cartridge_data;
        u8*                 cartridge_sram;

        cartridge_header    header;
        mbc_subsystem       cartridge_access;
    } cartridge_internal;

    public:

    cartridge() = delete;

    cartridge(u8* cartridge_data, u8* cartridge_sram, cartridge_header header)
    {
        cartridge_internal.cartridge_data = cartridge_data;
        cartridge_internal.cartridge_sram = cartridge_sram;
        cartridge_internal.header = header;
    }
};
