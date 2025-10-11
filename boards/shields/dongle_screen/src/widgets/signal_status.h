/*
 * Copyright (c) 2025 Your Name
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <lvgl.h>

struct zmk_widget_signal_status {
    lv_obj_t *obj;
    lv_obj_t *label;
    sys_snode_t node;
};

int zmk_widget_signal_status_init(struct zmk_widget_signal_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_signal_status_obj(struct zmk_widget_signal_status *widget);
