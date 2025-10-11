/*
 * Copyright (c) 2025 Your Name
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/event_manager.h>
#include <zmk/events/signal_state_changed.h>

struct zmk_widget_signal_status {
    lv_obj_t *obj;
    lv_obj_t *label;
    sys_snode_t node;
};

// 위젯 초기화
int zmk_widget_signal_status_init(struct zmk_widget_signal_status *widget, lv_obj_t *parent);

// 위젯 객체 반환
lv_obj_t *zmk_widget_signal_status_obj(struct zmk_widget_signal_status *widget);
