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

// 배터리 색상 결정 (밝은 색)
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

// 배터리 색상 어두운 버전 (줄어든 부분)
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) {
        return lv_color_hex(0x5F5CE7); // 슬립/완전 방전 어두운
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

/**
 * @brief 배터리 캔버스 그리기
 * - 전체 높이 5픽셀
 * - 왼쪽 고정, 오른쪽 줄어듦
 * - 밝은색 영역 + 어두운색 영역
 * - 양 끝 위/아래 1픽셀 검정 → 라운드 효과
 */
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    // 1. 전체 캔버스 밝은 색으로 초기화
    lv_canvas_fill_bg(canvas, battery_color(level), LV_OPA_COVER);

    // 2. 밝은 색 왼쪽 끝 위/아래 1픽셀 검정 → 라운드 느낌
    if (level > 0) {
        lv_canvas_set_px(canvas, 0, 0, lv_color_black()); // 왼쪽 위
        lv_canvas_set_px(canvas, 0, 4, lv_color_black()); // 왼쪽 아래
    }

    // 3. 잔량보다 오른쪽 영역을 어두운 색으로 덮기
    if (level < 102) {
        lv_draw_rect_dsc_t rect_fill_dsc;
        lv_draw_rect_dsc_init(&rect_fill_dsc);
        rect_fill_dsc.bg_color = battery_color_dark(level);
        rect_fill_dsc.border_width = 0;

        lv_canvas_draw_rect(canvas, level, 0, 102 - level, 5, &rect_fill_dsc);

        // 오른쪽 어두운 영역 양 끝 위/아래 1픽셀 검정 → 라운드 느낌
        lv_canvas_set_px(canvas, 101, 0, lv_color_black());   // 오른쪽 위
        lv_canvas_set_px(canvas, 101, 4, lv_color_black());   // 오른쪽 아래
    }
}

// 배터리 심볼 + 레이블 설정
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    bool reconnecting = is_peripheral_reconnecting(state.source, state.level);
    last_battery_levels[state.source] = state.level;

    if (reconnecting) {
#if CONFIG_DONGLE_SCREEN_IDLE_TIMEOUT_S > 0    
        LOG_INF("Peripheral %d reconnected (battery: %d%%), requesting screen wake", 
                state.source, state.level);
        brightness_wake_screen_on_reconnect();
#else 
        LOG_INF("Peripheral %d reconnected (battery: %d%%)", 
                state.source, state.level);
#endif
    }

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    // 레이블 색상
    lv_obj_set_style_text_color(label, battery_color(state.level), 0);

    if (state.level < 1) {
        lv_label_set_text(label, "sleep");
    } else {
        lv_label_set_text_fmt(label, "%u", state.level); // % 제거
    }

    lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(symbol);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);
}

// 모든 위젯 상태 업데이트
void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { 
        set_battery_symbol(widget->obj, state); 
    }
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

#if IS_ENABLED(CONFIG_Z
