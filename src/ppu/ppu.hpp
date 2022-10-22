#pragma     once

#include    "../common/common.hpp"
#include    "../ioregs/ioregs.hpp"
#include    "../client/client.hpp"

#define     VIDEO_RAM_ADDRESS_SIZE      8192
#define     VIDEO_RAM_TILES_SIZE        384
#define     VIDEO_RAM_TILEMAP_ROWS      32
#define     VIDEO_RAM_TILEMAP_COLUMNS   32

#define     OAM_ADDRESS_SIZE            160
#define     OAM_SPRITES_SIZE            40
#define     LINE_SPRITES_SIZE           10

struct gb_tile_t {
    u16_as_u8_t lines[8];
};

struct gb_vram_t {
    union {
        struct {
            gb_tile_t tile_data[VIDEO_RAM_TILES_SIZE];
            u8 tilemaps[2][VIDEO_RAM_TILEMAP_ROWS][VIDEO_RAM_TILEMAP_COLUMNS];
        } as_name;
        u8 as_raw[VIDEO_RAM_ADDRESS_SIZE] = { };
    } data;
    bool is_locked = false;
};

struct gb_obj_t {
    u8 y_position;
    u8 x_position;
    u8 tile_index;
    u8 flags;
};

struct line_obj_t {
    gb_obj_t obj;
    u8 index;
};

struct gb_oam_t {
    union {
        gb_obj_t as_sprites[OAM_SPRITES_SIZE];
        u8 as_raw[OAM_ADDRESS_SIZE] = { };
    } data;
    bool is_locked = false;
};

class gb_ppu_t {
    private:
    gb_vram_t& gb_vram; 
    gb_oam_t& gb_oam;
    gb_ioregs_t& gb_ioregs;
    bool ppu_enable = false;
    enum : u8 {
        MODE0_HBLANK,
        MODE1_VBLANK,
        MODE2_OAM_SCAN,
        MODE3_RENDERING
    } ppu_mode = MODE0_HBLANK;
    u8 wait_cycles = 0;
    struct {
        line_obj_t data[LINE_SPRITES_SIZE];
        u8 obj_count;
    } line_objs;
    client_screen_t client_screen;


    public:
    gb_ppu_t(gb_vram_t& gb_vram, gb_oam_t& gb_oam, gb_ioregs_t& gb_ioregs);

    //  This is called from the CPU, advancing the PPU by 4 T-cycles or 1 M-cycle, this could simulate the PPU rendering, performing OAM search or just idling.
    void cycle_advance();

    private:
    //  Increments LY at the end of H-Blank and during V-Blank, checking for LY=LYC and setting STAT interrupt flag if needed.
    void ly_increment();
    //  Handles switching between the 4 PPU modes, setting STAT interrupt flag and locking/unlocking OAM and VRAM.
    void mode_switch_handler();
    //  OAM scan is performed in one function call, then the PPU "idles" to simulate OAM scan period.
    void oam_scan();
    /*  An entire line is drawn in one function call, then the PPU "idles" to simulate rendering period. 
        Rendering [mode 3] can take longer than usual in some conditions, and H-Blank is shortened to compensate.
        The exact duration of mode 3 is difficult to ascertain, so we're just going with the "default" length for H-Blank and rendering phases. */
    void render_line();
    /*  Handles rendering of the background/window layers. */
    void render_bg_window();
    /*  After the background/window is rendered for a given scanline, we then render the sprites on 'top' of the background/window,
        taking sprite priority and other aspects into account. */
    void render_sprites();
    /*  Converts the 2-bit GB colour palettes to RGB8 colour values for rendering by the client. */
    void finalize_colours();
};