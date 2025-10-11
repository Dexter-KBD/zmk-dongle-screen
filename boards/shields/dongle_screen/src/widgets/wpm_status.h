#pragma once

#include <lvgl.h>
#include <zephyr/sys/slist.h>

// WPM 상태 위젯 구조체
struct zmk_widget_wpm_status {
    lv_obj_t *obj;               // 위젯 컨테이너
    lv_obj_t *wpm_label_title;   // "WPM" 텍스트 라벨
    lv_obj_t *wpm_label_value;   // WPM 숫자 라벨
    sys_snode_t node;            // 연결 리스트용 노드
};

// 위젯 초기화
int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent);

// 위젯 객체 반환
lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget);
