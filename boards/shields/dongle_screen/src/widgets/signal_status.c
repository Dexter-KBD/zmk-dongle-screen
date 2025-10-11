/*
 * Copyright (c) 2025 Your Name
 *
 * SPDX-License-Identifier: MIT
 */

#include "signal_status.h"
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/signal_state_changed.h> // 이벤트 구조체 정의 필요

#include <fonts.h>
#include <stdlib.h> // strtoul

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// --------------------
// 이벤트 구조체 정의 (가정)
// --------------------
struct zmk_signal_state_changed {
    int rssi;       // dBm
    int frequency;  // Hz
};

// --------------------
// 위젯 상태 구조체
// --------------------
struct signal_status_state {
    int rssi;       // dBm
    int frequency;  // Hz
};

// --------------------
// 웹컬러 -> LVGL 색상 변환
// --------------------
static lv_color_t lv_color_from_web(const char *hex) {
    uint32_t c = strtoul(hex + 1, NULL, 16); // "#RRGGBB" -> 0xRRGGBB
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    return lv_color_make(r, g, b);
}

// --------------------
// 이벤트 상태 가져오기
// --------------------
static struct signal_status_state get_state(const zmk_event_t *_eh) {
    const struct zmk_signal_state_changed *ev = as_zmk_signal_state_changed(_eh);
    return (struct signal_status_state){
        .rssi = ev ? ev->rssi : 0,
        .frequency = ev ? ev->frequency : 0
    };
}

// --------------------
// 위젯 업데이트
// --------------------
static void set_signal(struct zmk_widget_signal_status *widget, struct signal_status_state state) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%i dBm %i Hz", state.rssi, state.frequency);
    lv_label_set_text(widget->label, buf);

    // RSSI 기준 색상
    lv_color_t color;
    if (state.rssi < -80) {
        color = lv_color_from_web("#FF0000"); // 빨강
    } else if (state.rssi < -60) {
        color = lv_color_from_web("#FFFF00"); // 노랑
    } else {
        color = lv_color_from_web("#00FF00"); // 초록
    }

    lv_obj_set_style_text_color(widget->label, color, 0);
}

// --------------------
// 위젯 업데이트 콜백
// --------------------
static void signal_status_update_cb(struct signal_status_state state) {
    struct zmk_widget_signal_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_signal(widget, state);
    }
}

// --------------------
// ZMK 이벤트 리스너 등록
// --------------------
ZMK_DISPLAY_WIDGET_LISTENER(widget_signal_status, struct signal_status_state,
                            signal_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_signal_status, zmk_signal_state_changed);

// --------------------
// 위젯 초기화
// --------------------
int zmk_widget_signal_status_init(struct zmk_widget_signal_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 20);

    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_TOP_LEFT, 0, 0);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

// --------------------
// 위젯 객체 반환
// --------------------
lv_obj_t *zmk_widget_signal_status_obj(struct zmk_widget_signal_status *widget) {
    return widget->obj;
}
