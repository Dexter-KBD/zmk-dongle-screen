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
#include <zmk/event_manager.h>

#include "battery_status.h"
#include "../brightness.h"

// 소스 오프셋 설정 (중앙장치 포함 여부)
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
    lv_obj_t *symbol; // 배터리 캔버스
    lv_obj_t *label;  // 배터리 레이블
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 캔버스 버퍼
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][102 * 5];

// Peripheral reconnection tracking
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 퍼리퍼럴 초기화
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++) {
        last_battery_levels[i] = -1; // -1 = 초기 상태
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

// ----------------------------
// 배터리 색상 결정용 함수 (1% 단위 그라데이션)
// ----------------------------
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) {
        // 슬립/0% 상태
        return lv_color_hex(0x5f5ce7);
    }

    if (level <= 19) {
        // 1~19% 빨강(0xfb5e51) -> 노랑(0xffdb3c) 그라데이션
        uint8_t r_start = 0xfb, g_start = 0x5e, b_start = 0x51; // 빨강
        uint8_t r_end   = 0xff, g_end   = 0xdb, b_end   = 0x3c; // 노랑
        float t = (float)(level - 1) / (19 - 1); // 1~19%를 0~1로 정규화
        uint8_t r = r_start + (r_end - r_start) * t;
        uint8_t g = g_start + (g_end - g_start) * t;
        uint8_t b = b_start + (b_end - b_start) * t;
        return lv_color_make(r, g, b);
    }

    if (level <= 89) {
        // 20~89% 초록(0x72de75) -> 노랑(0xffdb3c) 그라데이션
        uint8_t r_start = 0x72, g_start = 0xde, b_start = 0x75; // 초록
        uint8_t r_end   = 0xff, g_end   = 0xdb, b_end   = 0x3c; // 노랑
        float t = (float)(level - 20) / (89 - 20); // 20~89%를 0~1로 정규화
        uint8_t r = r_start + (r_end - r_start) * t;
        uint8_t g = g_start + (g_end - g_start) * t;
        uint8_t b = b_start + (b_end - b_start) * t;
        return lv_color_make(r, g, b);
    }

    // 90~100% 초록
    return lv_color_hex(0x72de75);
}

// ----------------------------
// 배터리 캔버스 그리기
// ----------------------------
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_canvas_fill_bg(canvas, battery_color(level), LV_OPA_COVER);

    lv_draw_rect_dsc_t rect_fill_dsc;
    lv_draw_rect_dsc_init(&rect_fill_dsc);
    rect_fill_dsc.bg_color = lv_color_black();

    lv_canvas_set_px(canvas, 0, 0, lv_color_black());
    lv_canvas_set_px(canvas, 0, 4, lv_color_black());
    lv_canvas_set_px(canvas, 101, 0, lv_color_black());
    lv_canvas_set_px(canvas, 101, 4, lv_color_black());

    if (level <= 99 && level > 0) {
        lv_canvas_draw_rect(canvas, level, 1, 100 - level, 3, &rect_fill_dsc);
        lv_canvas_set_px(canvas, 100, 1, lv_color_black());
        lv_canvas_set_px(canvas, 100, 2, lv_color_black());
        lv_canvas_set_px(canvas, 100, 3, lv_color_black());
    }
}

// ----------------------------
// 배터리 심볼 + 레이블 설정
// ----------------------------
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    bool reconnecting = is_peripheral_reconnecting(state.source, state.level);
    last_battery_levels[state.source] = state.level;

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    // 텍스트 색상도 바 색과 동일
    lv_color_t col = battery_color(state.level);
    lv_obj_set_style_text_color(label, col, 0);

    if (state.level < 1) {
        lv_label_set_text(label, "sleep");
    } else {
        lv_label_set_text_fmt(label, "%4u%%", state.level);
    }

    lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(symbol);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);
}

// ----------------------------
// 모든 위젯에 상태 업데이트
// ----------------------------
void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { 
        set_battery_symbol(widget->obj, state); 
    }
}

// ----------------------------
// 퍼리퍼럴/중앙 배터리 이벤트 처리
// ----------------------------
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

// ----------------------------
// 디스플레이 위젯 리스너 등록
// ----------------------------
ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#endif
#endif

// ----------------------------
// 위젯 초기화
// ----------------------------
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 40);

    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_obj_t *battery_label = lv_label_create(widget->obj);

        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], 102, 5, LV_IMG_CF_TRUE_COLOR);

        lv_obj_align(image_canvas, LV_ALIGN_BOTTOM_MID, -60 + (i * 120), -8);
        lv_obj_align(battery_label, LV_ALIGN_TOP_MID, -60 + (i * 120), 0);

        lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);
        
        battery_objects[i] = (struct battery_object){
            .symbol = image_canvas,
            .label = battery_label,
        };
    }

    sys_slist_append(&widgets, &widget->node);
    init_peripheral_tracking();
    widget_dongle_battery_status_init();

    return 0;
}

// ----------------------------
// 다른 파일에서 접근 가능한 공개 함수
// ----------------------------
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
