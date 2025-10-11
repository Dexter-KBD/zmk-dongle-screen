/*
 * Copyright (c) 2025 Your Name
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <lvgl.h>

// Signal Status Widget 구조체
struct zmk_widget_signal_status {
    lv_obj_t *obj;     // 위젯 객체
    lv_obj_t *label;   // 라벨 객체
    sys_snode_t node;  // 싱글 링크드 리스트 노드
};

// 초기화 함수
int zmk_widget_signal_status_init(struct zmk_widget_signal_status *widget, lv_obj_t *parent);

// 위젯 객체 반환
lv_obj_t *zmk_widget_signal_status_obj(struct zmk_widget_signal_status *widget);
