/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * 배터리 상태 위젯 (LVGL 8 기준)
 * 
 * 변경 사항:
 * 1. 배터리 전체 어두운색 → 밝은색으로 덮는 방식으로 변경
 *    - 둥근 모서리가 어두운 색에 의해 침범되는 문제 해결
 * 2. 높이 20픽셀, 모서리 반지름 5픽셀 적용
 * 3. 숫자 라벨은 각 배터리 중앙에 표시
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

#include "battery_status.h"
#include "../brightness.h"

// ============================
// 배터리 막대 그리기 함수
// ============================

/**
 * @brief 배터리 막대 그리기
 * 
 * @param parent        부모 LVGL 객체
 * @param x             막대 시작 x 좌표
 * @param y             막대 시작 y 좌표
 * @param level         배터리 잔량 (0~100)
 * @param main_color_hex 밝은색(hex)
 * @param dark_color_hex 어두운색(hex)
 * @param text_color_hex 숫자 색상(hex)
 */
static void draw_battery_bar(lv_obj_t *parent, int x, int y, int level,
                             uint32_t main_color_hex, uint32_t dark_color_hex, uint32_t text_color_hex) {
    // ---------------------------
    // 설정
    // ---------------------------
    int width = 102;   // 배터리 폭
    int height = 20;   // 배터리 높이
    int radius = 5;    // 모서리 반지름

    // ---------------------------
    // 배경 막대 (전체 어두운색)
    // ---------------------------
    lv_obj_t *bg_bar = lv_obj_create(parent);
    lv_obj_set_size(bg_bar, width, height);                   // 크기 설정
    lv_obj_set_pos(bg_bar, x, y);                             // 위치 설정
    lv_obj_set_style_bg_color(bg_bar, lv_color_hex(dark_color_hex), 0); // 배경색 = 어두운색
    lv_obj_set_style_radius(bg_bar, radius, 0);              // 둥근 모서리
    lv_obj_set_style_border_width(bg_bar, 0, 0);             // 테두리 없음

    // ---------------------------
    // 전력 잔량에 따라 밝은색 막대 덮기
    // ---------------------------
    int fg_width = width * level / 100;                      // 밝은색 막대 폭 계산
    if (fg_width > 0) {                                      // 레벨이 0 이상일 때만 생성
        lv_obj_t *fg_bar = lv_obj_create(parent);
        lv_obj_set_size(fg_bar, fg_width, height);           // 폭 = 잔량 비율
        lv_obj_set_pos(fg_bar, x, y);                        // 왼쪽부터 시작
        lv_obj_set_style_bg_color(fg_bar, lv_color_hex(main_color_hex), 0); // 밝은색
        lv_obj_set_style_radius(fg_bar, radius, 0);          // 둥근 모서리
        lv_obj_set_style_border_width(fg_bar, 0, 0);         // 테두리 없음
    }

    // ---------------------------
    // 배터리 잔량 숫자 라벨
    // ---------------------------
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text_fmt(label, "%d%%", level);             // 숫자 포맷
    lv_obj_align_to(label, bg_bar, LV_ALIGN_CENTER, 0, 0);   // 배경 중앙에 위치
    lv_obj_set_style_text_color(label, lv_color_hex(text_color_hex), 0); // 텍스트 색
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);          // 중앙 정렬
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);       // 배경 투명
}

// ============================
// 배터리 디스플레이 생성
// ============================

/**
 * @brief 두 개 배터리 막대 표시
 * 
 * @param parent 부모 LVGL 객체
 */
void create_battery_display(lv_obj_t *parent) {
    // 왼쪽 배터리 (예: 중앙 배터리)
    draw_battery_bar(parent, 20, 20, 78, 0x08FB10, 0x04910A, 0xFFFFFF);

    // 오른쪽 배터리 (예: 퍼리퍼럴 배터리)
    draw_battery_bar(parent, 140, 20, 42, 0x5F5CE7, 0x3F3EC0, 0xFFFFFF);
}

// ============================
// 위젯 초기화
// ============================

/**
 * @brief 배터리 상태 위젯 초기화
 * 
 * @param widget 배터리 위젯 구조체
 * @param parent 부모 LVGL 객체
 * @return int 0 = 성공
 */
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 60);                  // 위젯 전체 크기
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE); // 스크롤 비활성화

    // 부모 객체 스타일 설정
    lv_obj_set_style_clip_corner(widget->obj, false, 0);    // 자식 클리핑 방지

    // 배터리 디스플레이 생성
    create_battery_display(widget->obj);

    return 0;
}

// ============================
// 공개 함수: 위젯 객체 반환
// ============================

lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
