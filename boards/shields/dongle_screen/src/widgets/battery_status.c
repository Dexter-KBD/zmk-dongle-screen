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

// 배터리 상태 구조체
struct battery_state {
    uint8_t source;
    uint8_t level;
};

// 위젯 오브젝트 구조체
struct battery_object {
    lv_obj_t *canvas;
    lv_obj_t *label;
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 캔버스 버퍼 (높이 18픽셀)
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][102 * 18];

// 마지막 배터리 상태 추적
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 퍼리퍼럴 초기화
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++) {
        last_battery_levels[i] = -1;
    }
}

/* ──────────────────────────────────────────────
 * 🔹 색상 지정 함수들 (일관된 형식)
 * ────────────────────────────────────────────── */
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

// ✅ 텍스트 색상 지정 함수 (배터리 잔량과 관계없이 흰색)
static lv_color_t battery_text_color(uint8_t level) {
    LV_UNUSED(level); // 나중에 조건부 색상 확장 가능
    return lv_color_hex(0xFFFFFF); // 기본 흰색
}

/* ──────────────────────────────────────────────
 * 🔹 배터리 막대 그리기 (radius 적용)
 *    - 왼쪽 직각, 오른쪽만 둥근 모양 유지
 * ────────────────────────────────────────────── */
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    // 1️⃣ 밝은 영역
    lv_draw_rect_dsc_t bright;
    lv_draw_rect_dsc_init(&bright);
    bright.radius = 6;
    bright.bg_color = battery_color(level);
    bright.border_width = 0;

    lv_canvas_draw_rect(canvas, 0, 0, 102, 18, &bright);

    // 2️⃣ 어두운 영역 — 왼쪽 직각, 오른쪽 둥글게
    if (level < 102) {
        lv_draw_rect_dsc_t dark;
        lv_draw_rect_dsc_init(&dark);
        dark.radius = 6;
        dark.bg_color = battery_color_dark(level);
        dark.border_width = 0;

        lv_area_t dark_area = { .x1 = level, .y1 = 0, .x2 = 101, .y2 = 17 };
        lv_canvas_draw_rect(canvas, dark_area.x1, dark_area.y1,
                            dark_area.x2 - dark_area.x1 + 1,
                            dark_area.y2 - dark_area.y1 + 1,
                            &dark);
    }
}

/* ──────────────────────────────────────────────
 * 🔹 배터리 상태 표시 (숫자 + 막대)
 * ────────────────────────────────────────────── */
static void set_battery_symbol(struct battery_object *obj, struct battery_state state) {
    last_battery_levels[state.source] = state.level;
    draw_battery(obj->canvas, state.level);

    // 중앙에 숫자 표시, 흰색 (함수로 색상 지정)
    lv_label_set_text_fmt(obj->label, "%u", state.level);
    lv_obj_set_style_text_color(obj->label, battery_text_color(state.level), 0);
    lv_obj_align(obj->label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_clear_flag(obj->canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(obj->label, LV_OBJ_FLAG_HIDDEN);
}

/* ──────────────────────────────────────────────
 * 🔹 상태 업데이트 & 이벤트 처리
 * ────────────────────────────────────────────── */
void battery_status_update_cb(struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;
    set_battery_symbol(&battery_objects[state.source], state);
}

static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){ .source = ev->source + SOURCE_OFFSET, .level = ev->state_of_charge };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){ .source = 0, .level = ev ? ev->state_of_charge : zmk_battery_state_of_charge() };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) {
    if (as_zmk_peripheral_battery_state_changed(eh)) return peripheral_battery_status_get_state(eh);
    else return central_battery_status_get_state(eh);
}

/* ──────────────────────────────────────────────
 * 🔹 LVGL 위젯 초기화 및 등록
 * ────────────────────────────────────────────── */
ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);
#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#endif
#endif

int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 40);

    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], 102, 18, LV_IMG_CF_TRUE_COLOR);
        lv_obj_clear_flag(image_canvas, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *battery_label = lv_label_create(image_canvas);
        lv_obj_set_size(battery_label, 102, 18);
        lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);

        battery_objects[i].canvas = image_canvas;
        battery_objects[i].label  = battery_label;
    }

    sys_slist_append(&widgets, &widget->node);
    init_peripheral_tracking();
    widget_dongle_battery_status_init();
    return 0;
}

// 공개 함수
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
