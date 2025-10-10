/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef OUTPUT_STATUS_H
#define OUTPUT_STATUS_H

#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <lvgl.h>

struct zmk_widget_output_status {
    lv_obj_t *obj;
    lv_obj_t *transport_label;  // USB/BLE 텍스트
    lv_obj_t *ble_label;        // BLE 프로필 번호
    sys_snode_t node;
};

// 상태 구조체 정의는 헤더에서만
struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

// 위젯 초기화
int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent);

// LVGL 오브젝트 반환
lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget);

#endif // OUTPUT_STATUS_H
