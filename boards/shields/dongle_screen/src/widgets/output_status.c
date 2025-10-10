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
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

#include "output_status.h"
#include <stdlib.h> // strtoul

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

lv_point_t selection_line_points[] = {{0, 0}, {13, 0}}; // will be replaced with lv_point_precise_t

struct output_status_state
{
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

// --------------------
// 웹컬러 -> LVGL 색상 변환
// --------------------
static lv_color_t lv_color_from_web(const char *hex)
{
    uint32_t c = strtoul(hex, NULL, 16);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    return lv_color_make(r, g, b);
}

// --------------------
// 이벤트 상태 가져오기
// --------------------
static struct output_status_state get_state(const zmk_event_t *_eh)
{
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
        .usb_is_hid_ready = zmk_usb_is_hid_ready()};
}

// --------------------
// 상태 라벨 설정
// --------------------
static void set_status_symbol(struct zmk_widget_output_status *widget, struct output_status_state state)
{
    const char *ble_color = "038eff"; // 기본 블루
    const char *usb_color = "cfa1f7"; // 기본 라벤더

    // USB 상태 색상
    if (state.usb_is_hid_ready == 0) {
        usb_color = "ff0000"; // 빨강
    } else {
        usb_color = "cfa1f7"; // 라벤더
    }

    // BLE 상태 색상
    if (state.active_profile_connected) {
        ble_color = "00ff00"; // 초록
    } else if (state.active_profile_bonded) {
        ble_color = "0000ff"; // 파랑
    } else {
        ble_color = "038eff"; // 기본 블루
    }

    // Transport 라벨
    char transport_text[50] = {};
    switch (state.selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        snprintf(transport_text, sizeof(transport_text), "> #%s USB#\n#%s BLE#", usb_color, ble_color);
        break;
    case ZMK_TRANSPORT_BLE:
        snprintf(transport_text, sizeof(transport_text), "#%s USB#\n> #%s BLE#", usb_color, ble_color);
        break;
    }
    lv_label_set_recolor(widget->transport_label, true);
    lv_obj_set_style_text_align(widget->transport_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(widget->transport_label, transport_text);

    // BLE 프로파일 숫자 표시 + 색상
    char ble_text[12];
    snprintf(ble_text, sizeof(ble_text), "%d", state.active_profile_index + 1);

    lv_label_set_text(widget->ble_label, ble_text);

    lv_color_t color;
    if (state.active_profile_connected) {
        color = lv_color_from_web("00ff00"); // 초록
    } else if (state.active_profile_bonded) {
        color = lv_color_from_web("0000ff"); // 파랑
    } else {
        color = lv_color_from_web("038eff"); // 기본 블루
    }
    lv_obj_set_style_text_color(widget->ble_label, color, 0);
}

// --------------------
// 업데이트 콜백
// --------------------
static void output_status_update_cb(struct output_status_state state)
{
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        set_status_symbol(widget, state);
    }
}

// --------------------
// ZMK 이벤트 리스너 등록
// --------------------
ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);

// --------------------
// 위젯 초기화
// --------------------
int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 77);

    widget->transport_label = lv_label_create(widget->obj);
    lv_obj_align(widget->transport_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    widget->ble_label = lv_label_create(widget->obj);
    lv_obj_align(widget->ble_label, LV_ALIGN_TOP_RIGHT, -10, 56);

    sys_slist_append(&widgets, &widget->node);

    widget_output_status_init();
    return 0;
}

// --------------------
// 위젯 객체 반환
// --------------------
lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget)
{
    return widget->obj;
}
