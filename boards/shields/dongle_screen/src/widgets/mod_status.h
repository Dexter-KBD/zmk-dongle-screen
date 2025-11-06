#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_mod_status {
    sys_snode_t node;   // Zephyr 싱글 리스트용 노드
    lv_obj_t *obj;      // LVGL 컨테이너 객체
    lv_obj_t *label;    // LVGL 라벨 객체
};

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);
