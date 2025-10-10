/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "output_status.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// USB 선택 시 화살표 색상 (오렌지)
static const char *usb_arrow_color = "ff7504";

static struct output_status_state get_state(const zmk_event_t *_eh)
{
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),                     // 0 = USB , 1 = BLE
        .active_profile_index = zmk_ble_active_profile_index(),            // 0-3 BLE profiles
        .active_profile_connected = zmk_ble_active_profile_is_connected(), // 0 = not connected, 1 = connected
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),        // 0 = BLE not bonded, 1 = bonded
        .usb_is_hid_ready = zmk_usb_is_hid_ready()};                       // 0 = not ready, 1 = ready
}

static void set_status_symbol(struct zmk_widget_output_status *widget, struct output_status_state state)
{
    // 기본 색상
    const char *ble_color = "038eff"; // Windows Bluetooth blue
    const char *usb_color = "cfa1f7"; // Lavender

    // USB 준비 상태
    if (!state.usb_is_hid_ready) {
        usb_color = "ff0000"; // 빨강
    }

    // BLE 상태
    if (state.active_profile_connected) {
        ble_color = "00ff00"; // 녹색
    } else if (state.active_profile_bonded) {
        ble_color = "0000ff"; // 파랑
    }

    // 표시 문자열 작성
    char transport_text[64];
    switch (state.selected_endpoint.transport) {
        case ZMK_TRANSPORT_USB:
            snprintf(transport_text, sizeof(transport_text),
                     "> #%s USB#\n#%s BLE#", usb_arrow_color, ble_color);
            break;
        case ZMK_TRANSPORT_BLE:
            snprintf(transport_text, sizeof(transport_text),
                     "#%s USB#\n> #%s BLE#", usb_color, ble_color);
            break;
    }

    // 라벨 설정
    lv_label_set_recolor(widget->transport_label, true);
    lv_obj_set_style_text_align(widget->transport_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(widget->transport_label, transport_text);

    // BLE 프로필 숫자 표시
    char ble_text[12];
    snprintf(ble_text, sizeof(ble_text), "#%s%d#", ble_color, state.active_profile_index + 1);
    lv_label_set_recolor(widget->ble_label, true);
    lv_label_set_text(widget->ble_label, ble_text);
}

static void output_status_update_cb(struct output_status_state state)
{
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_status_symbol(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 77);

    // USB/BLE 표시 라벨
    widget->transport_label = lv_label_create(widget->obj);
    lv_obj_align(widget->transport_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    // BLE 프로필 숫자 라벨
    widget->ble_label = lv_label_create(widget->obj);
    lv_obj_align(widget->ble_label, LV_ALIGN_TOP_RIGHT, -10, 56);

    sys_slist_append(&widgets, &widget->node);

    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget)
{
    return widget->obj;
}
