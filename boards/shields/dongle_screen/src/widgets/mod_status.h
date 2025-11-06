#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_mod_status {
    sys_snode_t node;      // linked list 노드
    lv_obj_t *obj;         // 메인 객체
    lv_obj_t *mod_label;   // 모드 상태 라벨
    lv_obj_t *caps_label;  // Caps Word 상태 라벨
};

// 초기화 함수
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent);

// 객체 반환 함수
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget);
