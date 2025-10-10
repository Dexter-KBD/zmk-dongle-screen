/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * 기능:
 * - USB/BLE transport 상태 표시
 * - 선택된 transport는 > 화살표로 강조
 * - 각 텍스트와 화살표 색상 적용 가능
 * - BLE 프로파일 숫자 표시 및 색상 적용
 *
 * 구현:
 * - transport_label 대신 USB/BLE 텍스트와 화살표를 별도 lv_label로 생성
 * - BLE 숫자도 별도 라벨로 스타일 적용
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

// --------------------
// 웹컬러(hex) -> LVGL 색상 변환
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
// 출력 상태 구조체
// --------------------
struct output_status_state
{
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

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
// 상태별 색상, 라벨 업데이트
// --------------------
static void set_status_symbol(struct zmk_widget_output_status *widget, struct output_status_state state)
{
    // --------------------
    // USB/BLE 텍스트 색상 결정
    // --------------------
    lv_color_t usb_color = lv_color_from_web(state.usb_is_hid_ready ? "cfa1f7" : "ff0000"); // USB 텍스트
    lv_color_t ble_color;
    if (state.active_profile_connected)
        ble_color = lv_color_from_web("00ff00"); // 초록
    else if (state.active_profile_bonded)
        ble_color = lv_color_from_web("0000ff"); // 파랑
    else
        ble_color = lv_color_from_web("038eff"); // 기본 블루

    // --------------------
    // 화살표 색상 결정
    // --------------------
    lv_color_t usb_arrow_color = lv_color_from_web("808080"); // 기본 회색
    lv_color_t ble_arrow_color = lv_color_from_web("808080"); // 기본 회색
    switch (state.selected_endpoint.transport)
    {
    case ZMK_TRANSPORT_USB:
        usb_arrow_color = lv_color_from_web("ff7504"); // 선택된 USB 화살표 오렌지
        break;
    case ZMK_TRANSPORT_BLE:
        ble_arrow_color = lv_color_from_web("ff00ff"); // 선택된 BLE 화살표 핑크
        break;
    }

    // --------------------
    // USB 화살표
    // --------------------
    if (!widget->usb_arrow)
    {
        widget->usb_arrow = lv_label_create(widget->obj);
        lv_obj_align(widget->usb_arrow, LV_ALIGN_TOP_RIGHT, -60, 10);
    }
    lv_label_set_text(widget->usb_arrow, ">");
    lv_obj_set_style_text_color(widget->usb_arrow, usb_arrow_color, 0);

    // --------------------
    // USB 텍스트
    // --------------------
    if (!widget->usb_label)
    {
        widget->usb_label = lv_label_create(widget->obj);
        lv_obj_align(widget->usb_label, LV_ALIGN_TOP_RIGHT, -40, 10);
    }
    lv_label_set_text(widget->usb_label, "USB");
    lv_obj_set_style_text_color(widget->usb_label, usb_color, 0);

    // --------------------
    // BLE 화살표
    // --------------------
    if (!widget->ble_arrow)
    {
        widget->ble_arrow = lv_label_create(widget->obj);
        lv_obj_align(widget->ble_arrow, LV_ALIGN_TOP_RIGHT, -60, 40);
    }
    lv_label_set_text(widget->ble_arrow, ">");
    lv_obj_set_style_text_color(widget->ble_arrow, ble_arrow_color, 0);

    // --------------------
    // BLE 텍스트
    // --------------------
    if (!widget->ble_label)
    {
        widget->ble_label = lv_label_create(widget->obj);
        lv_obj_align(widget->ble_label, LV_ALIGN_TOP_RIGHT, -40, 40);
    }
    lv_label_set_text(widget->ble_label, "BLE");
    lv_obj_set_style_text_color(widget->ble_label, ble_color, 0);

    // --------------------
    // BLE 프로파일 숫자
    // --------------------
    if (!widget->ble_num_label)
    {
        widget->ble_num_label = lv_label_create(widget->obj);
        lv_obj_align(widget->ble_num_label, LV_ALIGN_TOP_RIGHT, -10, 40);
    }
    char ble_num_text[4];
    snprintf(ble_num_text, sizeof(ble_num_text), "%d", state.active_profile_index + 1);
    lv_label_set_text(widget->ble_num_label, ble_num_text);

    lv_color_t ble_num_color;
    if (state.active_profile_connected)
        ble_num_color = lv_color_from_web("00ff00"); // 초록
    else if (state.active_profile_bonded)
        ble_num_color = lv_color_from_web("0000ff"); // 파랑
    else
        ble_num_color = lv_color_from_web("038eff"); // 기본 블루

    lv_obj_set_style_text_color(widget->ble_num_label, ble_num_color, 0);
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

    // 초기 라벨 생성
    widget->usb_arrow = NULL;
    widget->usb_label = NULL;
    widget->ble_arrow = NULL;
    widget->ble_label = NULL;
    widget->ble_num_label = NULL;

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
