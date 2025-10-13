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
    lv_obj_t *label;  // 배터리 레이블 (canvas와 같은 부모)
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 캔버스 버퍼
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][102 * 18]; // 높이 18픽셀

// Peripheral reconnection tracking
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 퍼리퍼럴 초기화
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++) {
        last_battery_levels[i] = -1;
    }
}

// 재연결 감지
static bool is_peripheral_reconnecting(uint8_t source, uint8_t new_level) {
    if (source >= (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET)) {
        return false;
    }
    int8_t previous_level = last_battery_levels[source];
    bool reconnecting = (previous_level < 1) && (new_level >= 1);
    if (reconnecting) {
        LOG_INF("Peripheral %d reconnection: %d%% -> %d%% (was %s)", 
                source, previous_level, new_level, 
                previous_level == -1 ? "never seen" : "disconnected");
    }
    return reconnecting;
}

// 배터리 색상 결정 (밝은 색)
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x08FB10);
}

// 배터리 색상 어두운 버전
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x3F3EC0);
    else if (level <= 15) return lv_color_hex(0xB20908);
    else if (level <= 30) return lv_color_hex(0xC76A00);
    else if (level <= 40) return lv_color_hex(0xB5B500);
    else return lv_color_hex(0x04910A);
}

/**
 * @brief 배터리 캔버스 그리기 (높이 18픽셀)
 * - 밝은 영역 전체, 어두운 영역 오른쪽만 radius 적용
 * - 왼쪽 직각, 오른쪽 둥글게
 */
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    // 밝은 영역
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 6;
    rect_dsc.bg_color = battery_color(level);
    rect_dsc.border_width = 0;
    lv_canvas_draw_rect(canvas, 0, 0, 102, 18, &rect_dsc);

    // 어두운 영역 (왼쪽 직각, 오른쪽 둥글게)
    if (level < 102) {
        lv_draw_rect_dsc_t rect_dark_dsc;
        lv_draw_rect_dsc_init(&rect_dark_dsc);
        rect_dark_dsc.radius = 6; // 오른쪽만 둥글게
        rect_dark_dsc.bg_color = battery_color_dark(level);
        rect_dark_dsc.border_width = 0;

        // LVGL에서는 radius를 전체 사각형에 적용 → 좌측은 직각 만들려면 x 좌표를 살짝 이동
        lv_canvas_draw_rect(canvas, level, 0, 102 - level, 18, &rect_dark_dsc);
    }
}

// 배터리 심볼 + 레이블 설정
static void set_battery_symbol(struct battery_object *obj, struct battery_state state) {
    last_battery_levels[state.source] = state.level;

    draw_battery(obj->canvas, state.level);

    // 숫자 중앙 정렬 (canvas 부모 기준)
    lv_label_set_text_fmt(obj->label, "%u", state.level);
    lv_obj_set_style_text_color(obj->label, lv_color_black(), 0);
    lv_obj_align(obj->label, LV_ALIGN_CENTER, lv_obj_get_x(obj->canvas) + 51 - obj->label->coords.x1, 0);

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
        .level = ev->state_of_charge,
    };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state) {
        .source = 0,
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
    };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) { 
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL) {
        return peripheral_battery_status_get_state(eh);
    } else {
        return central_battery_status_get_state(eh);
    }
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
        // 배터리 캔버스
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], 102, 18, LV_IMG_CF_TRUE_COLOR);
        lv_obj_clear_flag(image_canvas, LV_OBJ_FLAG_SCROLLABLE);

        // 숫자 레이블 (canvas와 같은 부모)
        lv_obj_t *battery_label = lv_label_create(widget->obj);

        lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);

        // 위치
        lv_obj_align(image_canvas, LV_ALIGN_BOTTOM_MID, -60 + (i * 120), -8);
        lv_obj_align(battery_label, LV_ALIGN_CENTER, -60 + (i * 120), -8);

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
