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

#define BATTERY_TEXT_COLOR_HEX 0xFFFFFF
#define BATTERY_WIDTH 90
#define BATTERY_HEIGHT 20
#define BATTERY_SPACING 140
#define CANVAS_WIDTH 112
#define CANVAS_HEIGHT 32
#define LEFT_MARGIN 6

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
};

struct battery_object {
    lv_obj_t *symbol;
    lv_obj_t *label;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][CANVAS_WIDTH * CANVAS_HEIGHT];

static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++)
        last_battery_levels[i] = -1;
}

// 배터리 색상
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x00E800);
}

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
    lv_draw_rect_dsc_t rect_dsc;

    // 흰색 배경 (가장 바깥)
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFFFFFF);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.border_width = 0;
    rect_dsc.radius = 8;
    lv_canvas_draw_rect(canvas, 2, 0, 102, 32, &rect_dsc);

    // 검정 공백
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0x000000);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 6;
    lv_canvas_draw_rect(canvas, 4, 2, 98, 28, &rect_dsc);

    // 어두운 배경
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = battery_color_dark(level);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 3;
    lv_canvas_draw_rect(canvas, 8, 6, BATTERY_WIDTH, BATTERY_HEIGHT, &rect_dsc);

    // 밝은 채움
    if (level > 0) {
        uint8_t width = (level > 100 ? 100 : level);
        uint8_t pixel_width = (uint8_t)((BATTERY_WIDTH * width) / 100);

        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = battery_color(level);
        rect_dsc.bg_opa = LV_OPA_COVER;
        rect_dsc.radius = 3;
        lv_canvas_draw_rect(canvas, 8, 6, pixel_width, BATTERY_HEIGHT, &rect_dsc);
    }
}

// 배터리 심볼 + 레이블
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    last_battery_levels[state.source] = state.level;

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    lv_obj_set_style_text_color(label, lv_color_hex(BATTERY_TEXT_COLOR_HEX), 0);

    if (state.level < 1) lv_label_set_text(label, "sleep");
    else lv_label_set_text_fmt(label, "%u", state.level);

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

static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){ .source = ev->source + SOURCE_OFFSET, .level = ev->state_of_charge };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){ .source = 0, .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge() };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) {
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL)
        return peripheral_battery_status_get_state(eh);
    else
        return central_battery_status_get_state(eh);
}

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
    lv_obj_set_size(widget->obj, 260, 40); // 컨테이너 폭 260

    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

        lv_obj_t *battery_label = lv_label_create(image_canvas);

        // 좌우 배터리 위치 조정
        int x_offset;
        if (i == 0) x_offset = -64; // 왼쪽 배터리
        else x_offset = 64;          // 오른쪽 배터리

        lv_obj_align(image_canvas, LV_ALIGN_CENTER, x_offset, 0);
        lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);

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
