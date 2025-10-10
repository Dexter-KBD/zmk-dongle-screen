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

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// 구조체 정의: 선택된 엔드포인트, BLE 프로필 상태, USB 상태
struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

// 현재 상태를 가져오는 함수
static struct output_status_state get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),                     // 0 = USB , 1 = BLE
        .active_profile_index = zmk_ble_active_profile_index(),            // 0-3 BLE profiles
        .active_profile_connected = zmk_ble_active_profile_is_connected(), // 연결 여부
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),        // 블루투스 바인딩 여부
        .usb_is_hid_ready = zmk_usb_is_hid_ready()};                       // USB HID 준비 여부
}

// 상태를 위젯에 표시하는 함수
static void set_status_symbol(struct zmk_widget_output_status *widget, struct output_status_state state) {
    // 색상 정의 (웹컬러 HEX)
    const char *ble_color = "038eff"; // 기본 윈도우 블루투스 색
    const char *usb_color = "cfa1f7"; // 기본 라벤더
    const char *usb_arrow_color = "ff7504"; // USB 선택 시 화살표 색상 (오렌지)

    char transport_text[64] = {};

    // USB 상태에 따른 색상 변경
    if (state.usb_is_hid_ready == 0) {
        usb_color = "ff0000"; // USB 준비 안됨 -> 빨강
    }

    // BLE 상태에 따른 색상 변경
    if (state.active_profile_connected == 1) {
        ble_color = "00ff00"; // 연결됨 -> 초록
    } else if (state.active_profile_bonded == 1) {
        ble_color = "0000ff"; // 바인딩됨 -> 파랑
    }

    // 선택된 엔드포인트에 따라 표시
    switch (state.selected_endpoint.transport) {
        case ZMK_TRANSPORT_USB:
            // USB 선택 시 화살표 오렌지, BLE는 기본 색
            snprintf(transport_text, sizeof(transport_text),
                     "> #%s# USB\n#%s# BLE", usb_arrow_color, ble_color);
            break;
        case ZMK_TRANSPORT_BLE:
            // BLE 선택 시 화살표 오렌지, USB는 기본 색
            snprintf(transport_text, sizeof(transport_text),
                     "#%s# USB\n> #%s# BLE", usb_color, usb_arrow_color);
            break;
    }

    // recolor 활성화
    lv_label_set_recolor(widget->transport_label, true);
    lv_obj_set_style_text_align(widget->transport_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(widget->transport_label, transport_text);

    // BLE 프로필 숫자 표시 (색 적용)
    char ble_text[12];
    snprintf(ble_text, sizeof(ble_text), "#%s#%d#", ble_color, state.active_profile_index + 1);
    lv_label_set_recolor(widget->ble_label, true);
    lv_label_set_text(widget->ble_label, ble_text);
}

// 위젯 상태 업데이트 콜백
static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_status_symbol(widget, state);
    }
}

// 디스플레이 리스너 및 이벤트 구독
ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);

// 위젯 초기화
int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
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

// 위젯 객체 반환
lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}
