#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/hid.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>

/**
 * 하나의 위젯에서 모디 상태와 Caps Word 상태를 동시에 표시하기 위한 구조체
 */
struct zmk_widget_mod_caps {
    sys_snode_t node;

    // LVGL 객체
    lv_obj_t *obj;          // 위젯 컨테이너
    lv_obj_t *mod_label;    // 모디 상태 표시 라벨
    lv_obj_t *caps_label;   // Caps Word 상태 표시 라벨
};

/**
 * 초기화
 * @param widget 구조체 포인터
 * @param parent LVGL 부모 객체
 * @return 0 성공
 */
int zmk_widget_mod_caps_init(struct zmk_widget_mod_caps *widget, lv_obj_t *parent);

/**
 * LVGL 객체 반환
 */
lv_obj_t *zmk_widget_mod_caps_obj(struct zmk_widget_mod_caps *widget);

