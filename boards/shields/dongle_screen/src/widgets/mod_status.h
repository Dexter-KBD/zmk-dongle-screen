#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

// mod_status 위젯 구조체
struct zmk_widget_mod_status {
    sys_snode_t node;
    lv_obj_t *obj;
};

// 초기화 및 객체 접근 함수 선언
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);
