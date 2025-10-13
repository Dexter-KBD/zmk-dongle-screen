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

// -------------------- 설정 값 --------------------
#define BATTERY_HEIGHT 20
#define BATTERY_WIDTH 96
#define BATTERY_RADIUS 4

#define BACK_BLACK_HEIGHT 24
#define BACK_BLACK_RADIUS 6

#define BACK_WHITE_HEIGHT 28
#define BACK_WHITE_RADIUS 8

#define BATTERY_TEXT_COLOR lv_color_hex(0x000000)
#define BATTERY_X_OFFSET 135

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
};

struct battery_object {
    lv_obj_t *symbol;
    lv_obj_t *label;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][(BATTERY_WIDTH + 8) * BACK_WHITE_HEIGHT];
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// -------------------- 초기화 --------------------
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++)
        last_battery_levels[i] = -1;
}

static bool is_peripheral_reconnecting(uint8_t source, uint8_t new_level) {
    if (source >= (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET)) return false;
    
    int8_t previous_level = last_battery_levels[source];
    bool reconnecting = (previous_level < 1) && (new_level >= 1);
    
    if (reconnecting) {
        LOG_INF("Peripheral %d reconnected: %d -> %d (was %s)", 
                source, previous_level, new_level, 
                previous_level == -1 ? "never seen" : "disconnected");
    }
    return reconnecting;
}

// -------------------- 배터리 색상 --------------------
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x08FB10);
}

static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xB20908);
    else if (level <= 30) return lv_color_hex(0xC76A00);
    else if (level <= 40) return lv_color_hex(0xB5B500);
    else return lv_color_hex(0x04910A);
}

// -------------------- 배터리 그리기 --------------------
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    const int margin = 2;
    const int inner_width = BATTERY_WIDTH;
    const int canvas_width = BATTERY_WIDTH + 8;
    
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);

    // 1. 흰색 바 (테두리 역할, 높이 28, radius 8)
    rect_dsc.radius = BACK_WHITE_RADIUS;
    rect_dsc.bg_color = lv_color_white();
    rect_dsc.border_width = 0;
    lv_canvas_draw_rect(canvas, (canvas_width - inner_width)/2 - margin, 0, inner_width + 2*margin, BACK_WHITE_HEIGHT, &rect_dsc);

    // 2. 검정 바 (흰색 바 위, 높이 24, radius 6)
    rect_dsc.radius = BACK_BLACK_RADIUS;
    rect_dsc.bg_color = lv_color_black();
    lv_canvas_draw_rect(canvas, (canvas_width - inner_width)/2, (BACK_WHITE_HEIGHT - BACK_BLACK_HEIGHT)/2, inner_width, BACK_BLACK_HEIGHT, &rect_dsc);

    // 3. 밝은색 배터리 잔량 표시 (높이 20, radius 4)
    rect_dsc.radius = BATTERY_RADIUS;
    rect_dsc.bg_color = battery_color(level);
    int level_width = (level * inner_width) / 100;
    lv_canvas_draw_rect(canvas, (canvas_width - inner_width)/2, (BACK_WHITE_HEIGHT - BATTERY_HEIGHT)/2, level_width, BATTERY_HEIGHT, &rect_dsc);
}

// -------------------- 심볼 + 레이블 --------------------
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    bool reconnecting = is_peripheral_reconnecting(state.source, state.level);
    last_battery_levels[state.source] = state.level;

    if (reconnecting) {
#if CONFIG_DONGLE_SCREEN_IDLE_TIMEOUT_S > 0    
        brightness_wake_screen_on_reconnect();
#endif
    }

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    lv_obj_set_style_text_color(label, BATTERY_TEXT_COLOR, 0);

    if (state.level < 1) lv_label_set_text(label, "sleep");
    else lv_label_set_text_fmt(label, "%u", state.level); // % 제거

    lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(symbol);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);
}

// -------------------- 이벤트 업데이트 --------------------
void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { 
        set_battery_symbol(widget->obj, state); 
    }
}

static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){ .source = ev->source + SOURCE_OFFSET, .level = ev->state_of_charge };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){ .source = 0, .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge() };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) { 
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL) return peripheral_battery_status_get_state(eh);
    else return central_battery_status_get_state(eh);
}

// -------------------- 디스플레이 리스너 --------------------
ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#endif
#endif

// -------------------- 위젯 초기화 --------------------
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 40);

    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_obj_t *battery_label = lv_label_create(widget->obj);

        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], BATTERY_WIDTH + 8, BACK_WHITE_HEIGHT, LV_IMG_CF_TRUE_COLOR);

        // 캔버스 중앙 정렬, 막대 위로 7픽셀 올리기
        lv_obj_align(image_canvas, LV_ALIGN_BOTTOM_MID, -BATTERY_X_OFFSET/2 + (i * BATTERY_X_OFFSET), -8 - 7);

        // 숫자 레이블 5픽셀 위로
        lv_obj_align(battery_label, LV_ALIGN_CENTER, -BATTERY_X_OFFSET/2 + (i * BATTERY_X_OFFSET), -5);

        lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);
        
        battery_objects[i] = (struct battery_object){ .symbol = image_canvas, .label = battery_label };
    }

    sys_slist_append(&widgets, &widget->node);
    init_peripheral_tracking();
    widget_dongle_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
