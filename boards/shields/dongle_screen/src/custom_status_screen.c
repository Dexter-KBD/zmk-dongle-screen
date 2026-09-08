/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"

extern void yads_diagnostic_mark(uint16_t color);

/* 기존 위젯 모듈의 공용 스타일 심볼은 유지한다. */
lv_style_t global_style;

/* 임시 분리 진단: 커스텀 위젯 없이 기본 폰트 한 줄만 그린다. */
lv_obj_t *zmk_display_status_screen()
{
    yads_diagnostic_mark(0x8410); /* gray: screen construction entered */
    lv_style_init(&global_style);

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "TEST");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label);

    yads_diagnostic_mark(0x07e0); /* green: minimal screen constructed */
    return screen;
}
