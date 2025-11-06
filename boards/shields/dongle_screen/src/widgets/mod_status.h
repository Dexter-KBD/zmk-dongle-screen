#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/events/keyboard_state_changed.h>

struct zmk_widget_mod_status {
    sys_snode_t node;
    lv_obj_t *mod_label;
    lv_obj_t *caps_label;
};

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);
