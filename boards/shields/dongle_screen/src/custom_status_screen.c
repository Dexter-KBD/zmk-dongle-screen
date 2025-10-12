/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Modified by Totem (2025) — added signal strength (RSSI) and advertisement frequency widgets
 */

#include "custom_status_screen.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// -----------------------------
// 기존 위젯 선언부
// -----------------------------
#if CONFIG_DONGLE_SCREEN_OUTPUT_ACTIVE
#include "widgets/output_status.h"
static struct zmk_widget_output_status output_status_widget;
#endif

#if CONFIG_DONGLE_SCREEN_LAYER_ACTIVE
#include "widgets/layer_status.h"
static struct zmk_widget_layer_status layer_status_widget;
#endif

#if CONFIG_DONGLE_SCREEN_BATTERY_ACTIVE
#include "widgets/battery_status.h"
static struct zmk_widget_dongle_battery_status dongle_battery_status_widget;
#endif

#if CONFIG_DONGLE_SCREEN_WPM_ACTIVE
#include "widgets/wpm_status.h"
static struct zmk_widget_wpm_status wpm_status_widget;
#endif

#if CONFIG_DONGLE_SCREEN_MODIFIER_ACTIVE
#include "widgets/mod_status.h"
static struct zmk_widget_mod_status mod_widget;
#endif

// -----------------------------
// ✅ 새로 추가: 신호강도 / 수신주기 위젯
// -----------------------------
#include "widgets/signal_status_widget.h"

// 신호 상태 위젯 구조체
static struct zmk_widget_signal_status signal_widget;

// 주기적 갱신용 워크 핸들러 선언
static void check_signal_timeout_handler(struct k_work *work);
static void periodic_rx_update_handler(struct k_work *work);

// 주기 작업 정의 (Zephyr의 비동기 타이머)
static K_WORK_DELAYABLE_DEFINE(signal_timeout_work, check_signal_timeout_handler);
static K_WORK_DELAYABLE_DEFINE(rx_periodic_work, periodic_rx_update_handler);

// -----------------------------
// 전역 스타일 (텍스트 및 배경용)
// -----------------------------
lv_style_t global_style;


// ========================================================================
// 🖥️ 화면 초기화 함수
// ========================================================================
lv_obj_t *zmk_display_status_screen()
{
    lv_obj_t *screen;

    // 기본 화면 객체 생성
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN);

    // 전역 텍스트 스타일 설정
    lv_style_init(&global_style);
    lv_style_set_text_color(&global_style, lv_color_white());
    lv_style_set_text_letter_space(&global_style, 1);
    lv_style_set_text_line_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

    // -----------------------------
    // 위젯별 초기화 및 위치 설정
    // -----------------------------
#if CONFIG_DONGLE_SCREEN_OUTPUT_ACTIVE
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_MID, 20, 10);
#endif

#if CONFIG_DONGLE_SCREEN_BATTERY_ACTIVE
    zmk_widget_dongle_battery_status_init(&dongle_battery_status_widget, screen);
    lv_obj_align(zmk_widget_dongle_battery_status_obj(&dongle_battery_status_widget), LV_ALIGN_BOTTOM_MID, 0, 0);
#endif

#if CONFIG_DONGLE_SCREEN_WPM_ACTIVE
    zmk_widget_wpm_status_init(&wpm_status_widget, screen);
    lv_obj_align(zmk_widget_wpm_status_obj(&wpm_status_widget), LV_ALIGN_TOP_LEFT, 20, 20);
#endif

#if CONFIG_DONGLE_SCREEN_LAYER_ACTIVE
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_CENTER, 0, 0);
#endif

#if CONFIG_DONGLE_SCREEN_MODIFIER_ACTIVE
    zmk_widget_mod_status_init(&mod_widget, screen);
    lv_obj_align(zmk_widget_mod_status_obj(&mod_widget), LV_ALIGN_CENTER, 0, 35);
#endif

    // -----------------------------
    // ✅ 신호강도 / 수신주기 위젯 추가
    // -----------------------------
    zmk_widget_signal_status_init(&signal_widget, screen);

    // 화면 오른쪽 하단에 배치
    lv_obj_align(zmk_widget_signal_status_obj(&signal_widget), LV_ALIGN_BOTTOM_RIGHT, -5, -5);

    // 신호 모니터링 주기 시작
    k_work_schedule(&signal_timeout_work, K_SECONDS(5));  // 5초마다 신호 유효성 확인
    k_work_schedule(&rx_periodic_work, K_SECONDS(1));     // 1초마다 RSSI / 주기 갱신

    return screen;
}


// ========================================================================
// 🛰️ 주기적 신호 체크 및 업데이트 처리
// ========================================================================

// 5초마다 신호 유효성 확인
static void check_signal_timeout_handler(struct k_work *work)
{
    // 신호 유효성 검사 (신호 끊김 감지 등)
    zmk_widget_signal_status_check_timeout(&signal_widget);

    // 다시 5초 후 실행 예약
    k_work_schedule(&signal_timeout_work, K_SECONDS(5));
}

// 1초마다 RSSI 및 수신 주기 정보 갱신
static void periodic_rx_update_handler(struct k_work *work)
{
    // 위젯 내부의 주기적 업데이트 로직 실행
    zmk_widget_signal_status_periodic_update(&signal_widget);

    // 다시 1초 후 실행 예약
    k_work_schedule(&rx_periodic_work, K_SECONDS(1));
}
