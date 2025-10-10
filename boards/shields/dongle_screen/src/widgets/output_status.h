/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZMK_WIDGET_OUTPUT_STATUS_H
#define ZMK_WIDGET_OUTPUT_STATUS_H

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>

struct zmk_widget_output_status {
    lv_obj_t *obj;

    lv_obj_t *transport_label; // USB/ BLE 표시 라벨
    lv_obj_t *ble_label;       // BLE 프로필 인덱스 표시

    sys_snode_t node;          // linked list 노드
};

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_is_hid_ready;
};

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget);

#endif /* ZMK_WIDGET_OUTPUT_STATUS_H */
