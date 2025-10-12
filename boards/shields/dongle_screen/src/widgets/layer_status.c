/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include "fonts.h" // TmoneyRound_40 선언 포함 (한글 글리프 포함)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state {
    uint8_t index;
    const char *label;
};

static void set_layer_symbol(lv_obj_t *obj, struct layer_status_state state) {
    // 텍스트 설정
    char text[16] = {}; // 충분히 큰 버퍼
    if (state.label == NULL) {
        sprintf(text, "%i", state.index);
    } else {
        snprintf(text, sizeof(text), "%s", state.label);
    }
    lv_label_set_text(obj, text);

    // 레이어별 색상 지정
    lv_color_t color;
    switch (state.index) {
        case 0: color = lv_color_hex(0xA8E6CF); break; // 민트
        case 1: color = lv_color_hex(0xDCEDC1); break; // 연한 그린
        case 2: color = lv_color_hex(0xFFD3B6); break; // 살구
        case 3: color = lv_color_hex(0xFFAAA5); break; // 코랄 핑크
        case 4: color = lv_color_hex(0xFFE082); break; // 크림 옐로우
        case 5: color = lv_color_hex(0xDCEDC1); break; // 연한 그린
        case 6: color = lv_color_hex(0xB3E5FC); break; // 하늘색
        case 7: color = lv_color_hex(0xF8BBD0); break; // 로즈 핑크
        case 8: color = lv_color_hex(0xFFE082); break; // 크림 옐로우
        default: color = lv_color_hex(0xFFFFFF); break; // 흰색
    }

    // 텍스트 색상 적용
    lv_obj_set_style_text_color(obj, color, 0);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_layer_symbol(widget->obj, state);
    }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index,
        .label = zmk_keymap_layer_name(index)
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);                   // 라벨 생성
    lv_obj_set_style_text_font(widget->obj, &TmoneyRound_40, 0); // 한글 지원 글꼴 적용

    sys_slist_append(&widgets, &widget->node);

    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}
