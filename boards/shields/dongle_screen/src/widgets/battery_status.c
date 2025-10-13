/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

#include "battery_status.h"
#include "../brightness.h"

// ============================
// 배터리 막대 그리기 함수
// ============================

static void draw_battery_bar(lv_obj_t *parent, int x, int y, int level,
                             uint32_t main_color_hex, uint32_t dark_color_hex, uint32_t text_color_hex) {
    // 배터리 막대 크기
    int width = 102;
    int height = 18;

    // 부모 객체가 막대 잘리지 않도록 설정
    lv_obj_set_clip_corner(parent, false);

    // ---- 밝은 영역 (전체 둥근 모서리) ----
    lv_obj_t *bg_bar = lv_obj_create(parent);
    lv_obj_set_size(bg_bar, width, height);
    lv_obj_set_pos(bg_bar, x, y);
    lv_obj_set_style_bg_color(bg_bar, lv_color_hex(main_color_hex), 0);
    lv_obj_set_style_radius(bg_bar, height / 2, 0);
    lv_obj_set_style_border_width(bg_bar, 0, 0);

    // ---- 어두운 덮음 부분 (왼쪽 직각, 오른쪽 둥글게) ----
    if (level < 100) {
        int dark_width = width * (100 - level) / 100;

        lv_obj_t *dark_bar = lv_obj_create(parent);
        lv_obj_set_size(dark_bar, dark_width, height);
        lv_obj_set_pos(dark_bar, x + width - dark_width, y);
        lv_obj_set_style_bg_color(dark_bar, lv_color_hex(dark_color_hex), 0);
        lv_obj_set_style_border_width(dark_bar, 0, 0);
        lv_obj_set_style_radius(dark_bar, height / 2, 0);
        lv_obj_set_style_clip_corner(dark_bar, true);
    }

    // ---- 중앙 숫자 표시 ----
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text_fmt(label, "%d%%", level);
    lv_obj_align_to(label, bg_bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(text_color_hex), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
}

// ============================
// 예시: 배터리 막대 2개 표시
// ============================

void create_battery_display(lv_obj_t *parent) {
    // 부모 객체 클리핑 방지 (두 막대 다 보이게)
    lv_obj_set_clip_corner(parent, false);

    // 왼쪽 배터리 (예: 중앙 배터리)
    draw_battery_bar(parent, 20, 20, 78, 0x08FB10, 0x04910A, 0xFFFFFF);

    // 오른쪽 배터리 (예: 퍼리퍼럴 배터리)
    draw_battery_bar(parent, 140, 20, 42, 0x5F5CE7, 0x3F3EC0, 0xFFFFFF);
}

// ============================
// 위젯 초기화
// ============================

int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 60);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    // 부모 객체가 자식 클리핑 안 하게 설정
    lv_obj_set_clip_corner(widget->obj, false);

    // 배터리 디스플레이 생성
    create_battery_display(widget->obj);

    return 0;
}

// 공개 함수
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
