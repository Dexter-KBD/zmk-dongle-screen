/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>

#include "wpm_status.h"
#include <fonts.h>
#include <stdlib.h> // strtoul

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct wpm_status_state {
    int wpm;
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
static struct wpm_status_state get_state(const zmk_event_t *_eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(_eh);

    return (struct wpm_status_state){
        .wpm = ev ? ev->state : 0
    };
}

// --------------------
// WPM 값별 색상 적용 + 줄 간격 설정
// --------------------
static void set_wpm(struct zmk_widget_wpm_status *widget, struct wpm_status_state state) {
    char wpm_text[12];
    snprintf(wpm_text, sizeof(wpm_text), "%i", state.wpm);
    lv_label_set_text(widget->wpm_label_value, wpm_text);

    // 숫자 색상: 구간별 적용
    lv_color_t color;
    if (state.wpm < 100) {
        color = lv_color_from_web("#FF7504"); // 오렌지
    } else if (state.wpm < 150) {
        color = lv_color_from_web("#FFFF00"); // 노랑
    } else {
        color = lv_color_from_web("#00FF00"); // 초록
    }
    lv_obj_set_style_text_color(widget->wpm_label_value, color, 0);

    // 왼쪽 정렬 + 줄 간격 (조금 띄움)
    lv_obj_set_style_text_align(widget->wpm_label_value, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_line_space(widget->wpm_label_value, 6, 0); // 줄 간격 추가
}

// --------------------
// 업데이트 콜백
// --------------------
static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_wpm(widget, state);
    }
}

// --------------------
// ZMK 이벤트 리스너 등록
// --------------------
ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            wpm_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

// --------------------
// 위젯 초기화
// --------------------
int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent) {
    // 컨테이너 생성
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 77);

    // --------------------
    // "WPM" 타이틀 (웹코드로 변경 가능)
    // --------------------
    widget->wpm_label_title = lv_label_create(widget->obj);
    lv_label_set_text(widget->wpm_label_title, "WPM");
    lv_obj_set_style_text_color(widget->wpm_label_title, lv_color_from_web("#FFE082"), 0); // 흰색 (웹코드로 변경 가능)
    lv_obj_set_style_text_align(widget->wpm_label_title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(widget->wpm_label_title, LV_ALIGN_TOP_LEFT, 0, 0);

    // --------------------
    // 숫자 라벨 (줄 간격 포함)
    // --------------------
    widget->wpm_label_value = lv_label_create(widget->obj);
    lv_label_set_text(widget->wpm_label_value, "0"); // 초기값
    lv_obj_set_style_text_align(widget->wpm_label_value, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_line_space(widget->wpm_label_value, 20, 0); // 줄 간격
    lv_obj_align(widget->wpm_label_value, LV_ALIGN_TOP_LEFT, 0, 20); // 아래쪽으로 배치

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_status_init();
    return 0;
}

// --------------------
// 객체 반환
// --------------------
lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget) {
    return widget->obj;
}
