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

// 중앙 장치 포함 여부에 따른 소스 오프셋
#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
    #define SOURCE_OFFSET 1
#else
    #define SOURCE_OFFSET 0
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source; // 0=중앙, 1..=퍼리퍼럴
    uint8_t level;
};

struct battery_object {
    lv_obj_t *canvas;
    lv_obj_t *label;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][102 * 18];

static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++) {
        last_battery_levels[i] = -1;
    }
}

static bool is_peripheral_reconnecting(uint8_t source, uint8_t new_level) {
    if (source >= (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET)) return false;
    int8_t previous_level = last_battery_levels[source];
    bool reconnecting = (previous_level < 1) && (new_level >= 1);
    if (reconnecting) {
        LOG_INF("Peripheral %d reconnected: %d%% -> %d%%", source, previous_level, new_level);
    }
    return reconnecting;
}

static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x08FB10);
}

static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x3F3EC0);
    else if (level <= 15) return lv_color_hex(0xB20908);
    else if (level <= 30) return lv_color_hex(0xC76A00);
    else if (level <= 40) return lv_color_hex(0xB5B500);
    else return lv_color_hex(0x04910A);
}

// 배터리 바 그리기
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    // 밝은 영역
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 6;
    rect_dsc.bg_color = battery_color(level);
    rect_dsc.border_width = 0;
    lv_canvas_draw_rect(canvas, 0, 0, 102, 18, &rect_dsc);

    // 어두운 영역: 왼쪽 직각, 오른쪽 radius
    if (level < 102) {
        lv_draw_rect_dsc_t rect_dark_dsc;
        lv_draw_rect_dsc_init(&rect_dark_dsc);
        rect_dark_dsc.radius = 0; // 왼쪽 직각
        rect_dark_dsc.bg_color = battery_color_dark(level);
        rect_dark_dsc.border_width = 0;
        lv_canvas_draw_rect(canvas, level, 0, 102 - level, 18, &rect_dark_dsc);
    }
}

// 배터리 심볼 + 레이블
static void set_battery_symbol(struct battery_object *obj, struct battery_state state) {
    last_battery_levels[state.source] = state.level;
    draw_battery(obj->canvas, state.level);

    lv_label_set_text_fmt(obj->label, "%u", state.level);
    lv_obj_set_style_text_color(obj->label, lv_color_black(), 0);
    lv_obj_align(obj->label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_clear_flag(obj->canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(obj->label, LV_OBJ_FLAG_HIDDEN);
}

// 업데이트 콜백
void battery_status_update_cb(struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;
    set_battery_symbol(&battery_objects[state.source], state);
}

// 이벤트 처리
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

// 위젯 리스너
ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#endif
#endif

// 위젯 초기화
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 40);

    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        lv_obj_t *canvas = lv_canvas_create(widget->obj);
        lv_canvas_set_buffer(canvas, battery_image_buffer[i], 102, 18, LV_IMG_CF_TRUE_COLOR);
        lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(canvas);
        lv_obj_set_size(label, 102, 18);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        // 각 배터리 x 위치 조정 (양쪽)
        int x_offset = -60 + i * 120;
        lv_obj_align(canvas, LV_ALIGN_BOTTOM_MID, x_offset, -8);

        lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);

        battery_objects[i].canvas = canvas;
        battery_objects[i].label  = label;
    }

    sys_slist_append(&widgets, &widget->node);
    init_peripheral_tracking();
    widget_dongle_battery_status_init();
    return 0;
}

lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
