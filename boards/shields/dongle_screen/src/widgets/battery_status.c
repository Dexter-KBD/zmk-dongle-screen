/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/split/central.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>

#include "battery_status.h"
#include "../brightness.h"

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
    #define SOURCE_OFFSET 1
#else
    #define SOURCE_OFFSET 0
#endif

#define BATTERY_TEXT_COLOR_HEX 0xFFFFFF  // 흰색 텍스트
#define BATTERY_WIDTH 95                 // 가로 길이 95픽셀
#define BATTERY_HEIGHT 20

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
};

struct battery_object {
    lv_obj_t *symbol;
    lv_obj_t *label;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][(BATTERY_WIDTH + 8) * BATTERY_HEIGHT];

static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++)
        last_battery_levels[i] = -1;
}

static bool is_peripheral_reconnecting(uint8_t source, uint8_t new_level) {
    if (source >= (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET)) return false;
    int8_t previous_level = last_battery_levels[source];
    bool reconnecting = (previous_level < 1) && (new_level >= 1);
    if (reconnecting) {
        LOG_INF("Peripheral %d reconnection: %d%% -> %d%%", source, previous_level, new_level);
    }
    return reconnecting;
}

// 밝은색 막대
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x00FF00);
}

// 어두운 배경
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xB20908);
    else if (level <= 30) return lv_color_hex(0xC76A00);
    else if (level <= 40) return lv_color_hex(0xB5B500);
    else return lv_color_hex(0x04910A);
}

// 배터리 캔버스 그리기
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);

    // 1️⃣ 외부 흰색 테두리
    lv_draw_rect_dsc_t rect_white;
    lv_draw_rect_dsc_init(&rect_white);
    rect_white.bg_color = lv_color_white();
    rect_white.bg_opa = LV_OPA_COVER;
    rect_white.border_width = 0;
    rect_white.radius = 9;
    lv_canvas_draw_rect(canvas, 0, 0, BATTERY_WIDTH + 8, BATTERY_HEIGHT, &rect_white);

    // 2️⃣ 검정 테두리
    lv_draw_rect_dsc_t rect_black;
    lv_draw_rect_dsc_init(&rect_black);
    rect_black.bg_color = lv_color_black();
    rect_black.bg_opa = LV_OPA_COVER;
    rect_black.border_width = 0;
    rect_black.radius = 7;
    lv_canvas_draw_rect(canvas, 4, 0, BATTERY_WIDTH + 0, BATTERY_HEIGHT, &rect_black);

    // 3️⃣ 어두운 막대 배경
    lv_draw_rect_dsc_t rect_bg_dsc;
    lv_draw_rect_dsc_init(&rect_bg_dsc);
    rect_bg_dsc.bg_color = battery_color_dark(level);
    rect_bg_dsc.bg_opa = LV_OPA_COVER;
    rect_bg_dsc.border_width = 0;
    rect_bg_dsc.radius = 5;
    lv_canvas_draw_rect(canvas, 4, 0, BATTERY_WIDTH, BATTERY_HEIGHT, &rect_bg_dsc);

    // 4️⃣ 밝은 막대 잔량 표시
    if (level > 0) {
        uint8_t width = (level > 100 ? 100 : level);
        uint8_t pixel_width = (uint8_t)((BATTERY_WIDTH * width) / 100);

        lv_draw_rect_dsc_t rect_fill_dsc;
        lv_draw_rect_dsc_init(&rect_fill_dsc);
        rect_fill_dsc.bg_color = battery_color(level);
        rect_fill_dsc.bg_opa = LV_OPA_COVER;
        rect_fill_dsc.border_width = 0;
        rect_fill_dsc.radius = 5;
        lv_canvas_draw_rect(canvas, 4, 0, pixel_width, BATTERY_HEIGHT, &rect_fill_dsc);
    }
}

// 배터리 심볼 + 레이블 설정
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    last_battery_levels[state.source] = state.level;

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    lv_obj_set_style_text_color(label, lv_color_hex(BATTERY_TEXT_COLOR_HEX), 0);

    if (state.level < 1) lv_label_set_text(label, "sleep");
    else lv_label_set_text_fmt(label, "%u", state.level); // % 제거

    // 배터리 막대 중앙에 맞춤
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(symbol);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);
}

void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_symbol(widget->obj, state);
    }
}

// ... 이하 기존 이벤트 처리 및 위젯 초기화 코드는 동일 ...
