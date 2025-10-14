/*******************************************************************************
 * Size: 16 px
 * Bpp: 4
 * Opts: --bpp 4 --size 16 --lcd --stride 1 --align 1 --font SF-Pro.ttf --range 48-57 --format lvgl -o apple_16.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#ifndef APPLE_16
#define APPLE_16 1
#endif

#if APPLE_16

/*-----------------
 *    BITMAPS
 *----------------*/

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* ... 기존 glyph_bitmap 내용 그대로 ... */
};

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 155, .box_w = 30, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 114, .box_w = 18, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 145, .box_w = 27, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 285, .adv_w = 152, .box_w = 27, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 155, .box_w = 30, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 510, .adv_w = 150, .box_w = 30, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 635, .adv_w = 158, .box_w = 30, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 776, .adv_w = 140, .box_w = 27, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 866, .adv_w = 154, .box_w = 30, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1007, .adv_w = 158, .box_w = 30, .box_h = 11, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const lv_font_fmt_txt_cmap_t cmaps[] = {
    {
        .range_start = 48, .range_length = 10, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
static lv_font_fmt_txt_glyph_cache_t cache;
#endif

static const lv_font_fmt_txt_dsc_t font_dsc = {
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 1,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

/*-----------------
 *  PUBLIC FONT
 *----------------*/

const lv_font_t apple_16 = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 11,
    .base_line = 0,
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_HOR,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};

#endif /* APPLE_16 */
