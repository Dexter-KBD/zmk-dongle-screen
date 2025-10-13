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
    uint8_t source;      // 배터리 소스 (0=중앙, 1..=퍼리퍼럴)
    uint8_t level;       // 배터리 잔량 %
};

// 위젯 오브젝트 구조체
struct battery_object {
    lv_obj_t *canvas; // 배터리 캔버스
    lv_obj_t *label;  // 배터리 레이블
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 캔버스 버퍼
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][102 * 18]; // 높이 18픽셀

// Peripheral reconnection tracking
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 퍼리퍼럴 초기화
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++) {
        last_battery_levels[i] = -1; // 초기 상태
    }
}

// 배터리 색상 결정 (밝은 색)
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7); // 슬립/완전 방전
    if (level <= 15) return lv_color_hex(0xFA0D0B); // 빨강
    if (level <= 30) return lv_color_hex(0xF98300); // 주황
    if (level <= 40) return lv_color_hex(0xFFFF00); // 노랑
    return lv_color_hex(0x08FB10); // 초록
}

// 배터리 색상 어두운 버전 (줄어든 영역)
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x3F3EC0);
    if (level <= 15) return lv_color_hex(0xB20908);
    if (level <= 30) return lv_color_hex(0xC76A00);
    if (level <= 40) return lv_color_hex(0xB5B500);
    return lv_color_hex(0x04910A);
}

/**
 * @brief 배터리 캔버스 그리기 (높이 18픽셀, radius 6)
 */
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 6;
    rect_dsc.bg_color = battery_color(level);
    rect_dsc.border_width = 0;

    // 전체 밝은 영역
    lv_canvas_draw_rect(canvas, 0, 0, 102, 18, &rect_dsc);

    // 줄어드는 어두운 영역
    if (level < 102) {
        lv_draw_rect_dsc_t rect_dark_dsc;
        lv_draw_rect_dsc_init(&rect_dark_dsc);
        rect_dark_dsc.radius = 6;
        rect_dark_dsc.bg_color = battery_color_dark(level);
        rect_dark_dsc.border_width = 0;

        lv_canvas_draw_rect(canvas, level, 0, 102 - level, 18, &rect_dark_dsc);
    }
}

// 배터리 심볼 + 레이블 설정
static void set_battery_symbol(struct battery_object *obj, struct battery_state state) {
    last_battery_levels[state.source] = state.level;

    draw_battery(obj->canvas, state.level);

    // 숫자 레이블 배터리 중앙에 표시
    lv_label_set_text_fmt(obj->label, "%u", state.level);
    lv_obj_set_style_text_color(obj->label, lv_color_black(), 0);
    lv_obj_align(obj->label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_clear_flag(obj->canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(obj->label, LV_OBJ_FLAG_HIDDEN);
}

// 모든 위젯 상태 업데이트
void battery_status_update_cb(struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;
    set_battery_symbol(&battery_objects[state.source], state);
}

// 이벤트 기반 상태 가져오기
static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){
        .source = ev->source + SOURCE_OFFSET,
        .level  = ev->state_of_charge,
    };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){
        .source = 0,
        .level  = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
    };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) {
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL) {
        return peripheral_battery_status_get_state(eh);
    }
    return central_battery_status_get_state(eh);
}

// 디스플레이 위젯 리스너 등록
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

        // 라벨은 canvas가 아닌 부모 위젯에 생성하여 중앙 정렬 보장
        lv_obj_t *label = lv_label_create(widget->obj);
        lv_obj_set_size(label, 102, 18);
        lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, -60 + i * 120, -8 + 0); // 위치 조정
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

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

// 공개 함수
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
