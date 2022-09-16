#pragma     once

#include    "../common/common.hpp"

#define     VIDEO_RAM_ADDRESS_SIZE      8192
#define     VIDEO_RAM_TILES_SIZE        384
#define     VIDEO_RAM_TILEMAP_SIZE      1024

#define     OAM_ADDRESS_SIZE            160
#define     OAM_SPRITES_SIZE            (OAM_ADDRESS_SIZE / 4)

struct gb_vram_t {
    union {
        struct {
            struct {
                u16_as_u8_t lines[8];
            }       as_tiles[VIDEO_RAM_TILES_SIZE];
            u8 tilemap_9800[VIDEO_RAM_TILEMAP_SIZE];
            u8 tilemap_9C00[VIDEO_RAM_TILEMAP_SIZE];
        } as_name;
        u8 as_raw[VIDEO_RAM_ADDRESS_SIZE] = { };
    } data;
    bool is_locked = false;
};

struct gb_oam_t {
    union {
        struct {
            u8 y_pos;
            u8 x_pos;
            u8 tile_flags;
            u8 flags;
        } as_sprites[OAM_SPRITES_SIZE];
        u8 as_raw[OAM_ADDRESS_SIZE] = { };
    } data;
    bool is_locked = false;
};