#pragma once

#include <lvgl.h>
#include <zmk/display.h>
#include <zephyr/sys/slist.h>  // sys_snode_t용

/* ===========================
   모드 상태 위젯
   =========================== */
struct zmk_widget_mod_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *label;
};

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);

/* ===========================
   Caps Word 상태 위젯
   =========================== */
struct zmk_widget_caps_word_indicator {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_widget_caps_word_indicator_init(struct zmk_widget_caps_word_indicator *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_caps_word_indicator_obj(struct zmk_widget_caps_word_indicator *widget);
