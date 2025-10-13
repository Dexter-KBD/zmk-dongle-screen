/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * 배터리 상태 위젯 (LVGL 8 기준)
 *
 * 특징:
 * 1. 배터리 전체 = 어두운색
 * 2. 잔량만큼 밝은색으로 위에서 덮는 구조
 * 3. 높이 20픽셀, radius 5
 * 4. 각 배터리 중앙 숫자 표시 (% 제거)
 * 5. 잔량 기반 색상 적용
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

// ---------------------------
// 배터리 색상 결정 (밝은 색)
// ---------------------------
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) {
        return lv_color_hex(0x5F5CE7); // 슬립/완전 방전
    } else if (level <= 15) {
        return lv_color_hex(0xFA0D0B); // 빨강
    } else if (level <= 30) {
        return lv_color_hex(0xF98300); // 주황
    } else if (level <= 40) {
        return lv_color_hex(0xFFFF00); // 노랑
    } else {
        return lv_color_hex(0x08FB10); // 초록
    }
}

// ---------------------------
// 배터리 색상 결정 (어두운색)
// ---------------------------
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) {
        return lv_color_hex(0x5F5CE7); // 슬립 위랑 같은색
    } else if (level <= 15) {
        return lv_color_hex(0xB20908); // 빨강 어두운
    } else if (level <= 30) {
        return lv_color_hex(0xC76A00); // 주황 어두운
    } else if (level <= 40) {
        return lv_color_hex(0xB5B500); // 노랑 어두운
    } else {
        return lv_color_hex(0x04910A); // 초록 어두운
    }
}

// ============================
// 배터리 막대 그리기
// ============================
static void draw_battery_bar(lv_obj_t *parent, int x, int y, uint8_t level) {
    int width = 102;  // 배터리 폭
    int height = 20;  // 배터리 높이
    int radius = 5;   // 모서리 반지름

    lv_color_t main_color = battery_color(level);
    lv_color_t dark_color = battery_color_dark(level);

    // ---------------------------
    // 배경 막대 (어두운색)
    // ---------------------------
    lv_obj_t *bg_bar = lv_obj_create(parent);
    lv_obj_set_size(bg_bar, width, height);
    lv_obj_set_pos(bg_bar, x, y);
    lv_obj_set_style_bg_color(bg_bar, dark_color, 0);
    lv_obj_set_style_radius(bg_bar, radius, 0);
    lv_obj_set_style_border_width(bg_bar, 0, 0);
    lv_obj_clear_flag(bg_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bg_bar, LV_OBJ_FLAG_HIDDEN);

    // ---------------------------
    // 밝은색 막대 (잔량 비율)
    // ---------------------------
    int fg_width = width * level / 100;
    if (fg_width > 0) {
        lv_obj_t *fg_bar = lv_obj_create(parent);
        lv_obj_set_size(fg_bar, fg_width, height);
        lv_obj_set_pos(fg_bar, x, y);
        lv_obj_set_style_bg_color(fg_bar, main_color, 0);
        lv_obj_set_style_radius(fg_bar, radius, 0);
        lv_obj_set_style_border_width(fg_bar, 0, 0);
        lv_obj_clear_flag(fg_bar, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(fg_bar, LV_OBJ_FLAG_HIDDEN);
    }

    // ---------------------------
    // 숫자 라벨
    // ---------------------------
    lv_obj_t *label = lv_label_create(parent);
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", level);  // % 제거
    lv_label_set_text(label, buf);
    lv_obj_align_to(label, bg_bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN); // 항상 표시
}

// ============================
// 배터리 디스플레이 생성
// ============================
void create_battery_display(lv_obj_t *parent) {
    // 좌표 조정하여 부모 범위 안에 들어오도록
    draw_battery_bar(parent, 10, 10, 78);   // 왼쪽 배터리
    draw_battery_bar(parent, 130, 10, 42);  // 오른쪽 배터리
}

// ============================
// 위젯 초기화
// ============================
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 60);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(widget->obj, false, 0);

    create_battery_display(widget->obj);

    return 0;
}

// ============================
// 공개 함수: 위젯 객체 반환
// ============================
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
