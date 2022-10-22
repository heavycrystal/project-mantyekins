#include    "ppu.hpp"
#include <cstddef>
#include <iterator>

#define     DISPLAY_X_DIMENSIONS    160
#define     DISPLAY_Y_DIMENSIONS    144

#define     VBLANK_START_LINE       DISPLAY_Y_DIMENSIONS
#define     VBLANK_END_LINE         154
#define     OAM_SCAN_WAIT_CYCLES    20
#define     RENDERING_WAIT_CYCLES   43
#define     HBLANK_WAIT_CYCLES      51
#define     VBLANK_LINE_WAIT_CYCLES (OAM_SCAN_WAIT_CYCLES + RENDERING_WAIT_CYCLES + HBLANK_WAIT_CYCLES)

// if set to 0, both BG and window stop rendering and are set to whatever is mapped to colour 00, sprites can still render if enabled.
#define     LCDC_BG_WINDOW_ENABLE   0
// if set to 0, sprites/objects stop rendering.
#define     LCDC_OBJ_ENABLE         1
// 0 -> 8x8 sprites and 1 -> 8x16 sprites [the tile immediately after the sprite's tile is used for the 2nd tile]
#define     LCDC_OBJ_SIZE           2
#define     LCDC_BG_TILE_MAP        3
#define     LCDC_BG_WINDOW_TILES    4
#define     LCDC_WINDOW_ENABLE      5
#define     LCDC_WINDOW_TILE_MAP    6
#define     LCDC_LCD_ENABLE         7

#define     STAT_LY_EQUALS_LYC_FLAG         2
#define     STAT_HBLANK_MODE_INTERRUPT      3
#define     STAT_VBLANK_MODE_INTERRUPT      4       
#define     STAT_OAM_SCAN_MODE_INTERRUPT    5
#define     STAT_LY_EQUALS_LYC_INTERRUPT    6

#define     OBJ_X_POS_OFFSET    8
#define     OBJ_Y_POS_OFFSET    16
#define     OBJ_Y_SIZE_L        8
#define     OBJ_Y_SIZE_H        16

#define     OBJ_Y_SIZE_CURRENT      (BIT_IS_SET(this->gb_ioregs.as_name.lcdc, 2) ? OBJ_Y_SIZE_H : OBJ_Y_SIZE_L)
#define     SET_STAT_INTERRUPT(bit) if(BIT_IS_SET(this->gb_ioregs.as_name.stat, bit)) {   \
                                        SET_BIT(this->gb_ioregs.as_name.iflag, STAT_INTERRUPT);     \
                                    }

#define     GET_COLOUR_FROM_TILE(tile, x_offset, y_offset)  (GET_BIT(tile->lines[y_offset].as_u8.lower, 7 - x_offset) \
                                                            | (GET_BIT(tile->lines[y_offset].as_u8.upper, 7 - x_offset) << 1))


/*  This is to sort the line's sprites in "reverse order", so that the higher precedence sprites are drawn later and over any lower precedence sprites.
    The rules to resolve precedence between two sprites that both occupy a given pixel are:
    
        1. If the starting x-coordinates of the sprites are unequal, the one with the lower x-coordinate wins.
        2. If the starting x-coordinates are equal, the one with the lower index in OAM wins. */
bool operator < (line_obj_t const& lhs, line_obj_t const& rhs) {
    if(lhs.obj.x_position != rhs.obj.x_position) {
        return lhs.obj.x_position > rhs.obj.x_position;
    }
    return lhs.index > rhs.index;
};

gb_ppu_t::gb_ppu_t(gb_vram_t& gb_vram, gb_oam_t& gb_oam, gb_ioregs_t& gb_ioregs) : gb_vram(gb_vram), gb_oam(gb_oam), gb_ioregs(gb_ioregs) {}

void gb_ppu_t::ly_increment() {
    this->gb_ioregs.as_name.ly = (this->gb_ioregs.as_name.ly + 1) % VBLANK_END_LINE;
    //  The time period of LY == LYC is not exactly clear, so assuming it sets at the beginning of each line.
    if(this->gb_ioregs.as_name.ly == this->gb_ioregs.as_name.lyc) {
        SET_BIT(this->gb_ioregs.as_name.stat, STAT_LY_EQUALS_LYC_FLAG);
        SET_STAT_INTERRUPT(STAT_LY_EQUALS_LYC_INTERRUPT);
    } else {
        CLR_BIT(this->gb_ioregs.as_name.stat, STAT_LY_EQUALS_LYC_FLAG);
    }
}

void gb_ppu_t::mode_switch_handler() {
    switch(this->ppu_mode) {
        case MODE0_HBLANK: {
            ly_increment();
            if(this->gb_ioregs.as_name.ly == VBLANK_START_LINE) { // rendering phase has finished, enter V-Blank [mode 1]
                this->ppu_mode = MODE1_VBLANK;
                this->wait_cycles = VBLANK_LINE_WAIT_CYCLES;
                this->gb_ioregs.as_name.stat = (BIT_CUT(this->gb_ioregs.as_name.stat, 2, 6) << 2) | MODE1_VBLANK;
                SET_STAT_INTERRUPT(STAT_VBLANK_MODE_INTERRUPT);
            } else {    // still in rendering lines, enter OAM scan [mode 2]
                this->gb_oam.is_locked = true;
                this->ppu_mode = MODE2_OAM_SCAN;
                this->gb_ioregs.as_name.stat = (BIT_CUT(this->gb_ioregs.as_name.stat, 2, 6) << 2) | MODE2_OAM_SCAN;
                SET_STAT_INTERRUPT(STAT_OAM_SCAN_MODE_INTERRUPT);
            }
            break;
        }
        case MODE1_VBLANK: {
            ly_increment();
            if(this->gb_ioregs.as_name.ly == 0) {
                this->gb_oam.is_locked = true;
                this->ppu_mode = MODE2_OAM_SCAN;
                this->gb_ioregs.as_name.stat = (BIT_CUT(this->gb_ioregs.as_name.stat, 2, 6) << 2) | MODE2_OAM_SCAN;
                SET_STAT_INTERRUPT(STAT_OAM_SCAN_MODE_INTERRUPT);
            } else {
                this->wait_cycles = VBLANK_LINE_WAIT_CYCLES;
            }
            break;
        }
        case MODE2_OAM_SCAN: {
            this->gb_vram.is_locked = true;
            this->ppu_mode = MODE3_RENDERING;
            this->gb_ioregs.as_name.stat = (BIT_CUT(this->gb_ioregs.as_name.stat, 2, 6) << 2) | MODE3_RENDERING;
            break;
        }
        case MODE3_RENDERING: {
            this->gb_oam.is_locked = false;
            this->gb_vram.is_locked = false;
            this->ppu_mode = MODE0_HBLANK;
            this->wait_cycles = HBLANK_WAIT_CYCLES;
            break;
        }
    }
}

void gb_ppu_t::cycle_advance() {
    if(this->wait_cycles > 0) {
        if(this->wait_cycles-- == 0) {
            mode_switch_handler();
        }
        return;
    } else if(this->ppu_mode == MODE2_OAM_SCAN) {
        oam_scan();
        this->wait_cycles = OAM_SCAN_WAIT_CYCLES;
    } else if(this->ppu_mode == MODE3_RENDERING) {

    }
}

void gb_ppu_t::oam_scan() {
    this->line_objs.obj_count = 0;
    for(u8 oam_loop_var = 0; (oam_loop_var < OAM_SPRITES_SIZE) && (this->line_objs.obj_count < LINE_SPRITES_SIZE); oam_loop_var++) {
        //  The list of conditions for selection of an OAM entry is picked up from here: https://www.youtube.com/watch?v=HyzD8pNlpwI
        if((this->gb_oam.data.as_sprites[oam_loop_var].x_position != 0)
            && (this->gb_oam.data.as_sprites[oam_loop_var].y_position >= (this->gb_ioregs.as_name.ly + OBJ_Y_POS_OFFSET))
            && ((this->gb_oam.data.as_sprites[oam_loop_var].y_position + OBJ_Y_SIZE_CURRENT) > (this->gb_ioregs.as_name.ly + OBJ_Y_POS_OFFSET))) {
                memcpy(&this->line_objs.data[this->line_objs.obj_count].obj, &this->gb_oam.data.as_sprites[oam_loop_var], sizeof(gb_obj_t));
                this->line_objs.data[this->line_objs.obj_count].index = this->line_objs.obj_count;
                this->line_objs.obj_count = this->line_objs.obj_count + 1;
        }
    }
    std::sort(this->line_objs.data, this->line_objs.data + this->line_objs.obj_count);
}

//  We render an entire scanline at once for simplicity, so no need to bother with mid-scanline LCDC updates.
void gb_ppu_t::render_line() {
    if(BIT_IS_SET(this->gb_ioregs.as_name.lcdc, LCDC_BG_WINDOW_ENABLE)) {
        render_bg_window();
    } else {
        memset(this->client_screen.pixels[this->gb_ioregs.as_name.ly], 0, DISPLAY_X_DIMENSIONS * sizeof(u8) * 3);
    }

    if(BIT_IS_SET(this->gb_ioregs.as_name.lcdc, LCDC_OBJ_ENABLE)) {
        render_sprites();
    }
    this->wait_cycles = RENDERING_WAIT_CYCLES;
}

void gb_ppu_t::render_bg_window() {
    auto current_bg_tilemap = this->gb_vram.data.as_name.tilemaps[GET_BIT(this->gb_ioregs.as_name.lcdc, LCDC_BG_TILE_MAP)];
    auto current_window_tilemap = this->gb_vram.data.as_name.tilemaps[GET_BIT(this->gb_ioregs.as_name.lcdc, LCDC_WINDOW_TILE_MAP)];
    u8 current_tile_index;
    u8 x_offset;
    u8 y_offset;
    gb_tile_t* current_tile = nullptr;

    for(u8 line_x_position = 0; line_x_position < DISPLAY_X_DIMENSIONS; line_x_position++) {
        if(BIT_IS_SET(this->gb_ioregs.as_name.lcdc, LCDC_WINDOW_ENABLE)
        && (this->gb_ioregs.as_name.ly >= this->gb_ioregs.as_name.wy)
        && (line_x_position >= (this->gb_ioregs.as_name.wx - 7))) {
            current_tile_index = current_window_tilemap[((this->gb_ioregs.as_name.ly + this->gb_ioregs.as_name.scy - this->gb_ioregs.as_name.wy) & 0xFF) >> 3]
            [((line_x_position + this->gb_ioregs.as_name.scx - (this->gb_ioregs.as_name.wx - 7)) & 0xFF) >> 3];
            x_offset = ((line_x_position + this->gb_ioregs.as_name.scx - (this->gb_ioregs.as_name.wx - 7)) & 0xFF) % 8;
            y_offset = ((this->gb_ioregs.as_name.ly + this->gb_ioregs.as_name.scy - this->gb_ioregs.as_name.wy) & 0xFF) % 8;
        } else {
            current_tile_index = current_bg_tilemap[((this->gb_ioregs.as_name.scy + this->gb_ioregs.as_name.ly) & 0xFF) >> 3]
            [((this->gb_ioregs.as_name.scx + line_x_position) & 0xFF) >> 3];
            x_offset = ((this->gb_ioregs.as_name.scx + line_x_position) & 0xFF) % 8;
            y_offset = ((this->gb_ioregs.as_name.scy + this->gb_ioregs.as_name.ly) & 0xFF) % 8;
        }

        current_tile = &this->gb_vram.data.as_name.tile_data
        [(BIT_IS_SET(this->gb_ioregs.as_name.lcdc, LCDC_BG_WINDOW_TILES) ? current_tile_index : (256 + static_cast<int8_t>(current_tile_index)))];
        this->client_screen.pixels[this->gb_ioregs.as_name.ly][line_x_position].red = GET_COLOUR_FROM_TILE(current_tile, x_offset, y_offset);
    }
}

void gb_ppu_t::finalize_colours() {
    for(u8 line_x_position = 0; line_x_position < DISPLAY_X_DIMENSIONS; line_x_position++) {

    }
}

