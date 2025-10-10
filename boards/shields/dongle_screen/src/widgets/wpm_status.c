/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include "wpm_status.h"
#include <lvgl.h>

// 위젯 리스트 초기화
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct wpm_status_state {
    struct tm timeinfo; // 표시용 시간 구조체
};

/*
 * 현재 시스템 업타임(uptime)을 기반으로 시각 생성
 * RTC가 없으므로 실제 "현재 시각"이 아니라 시스템 부팅 이후 경과 시간입니다.
 * 한국 표준시(KST, UTC+9) 보정을 포함합니다.
 */
static struct wpm_status_state get_state(const zmk_event_t *_eh)
{
    // 부팅 후 경과 시간(ms 단위)
    uint64_t uptime_ms = k_uptime_get();

    // 초 단위로 변환
    time_t uptime_sec = uptime_ms / 1000;

    // UTC+9 보정 (단순히 9시간 추가)
    uptime_sec += 9 * 3600;

    // 수동으로 시/분/초 계산
    struct tm tm_info;
    tm_info.tm_hour = (uptime_sec / 3600) % 24;
    tm_info.tm_min = (uptime_sec / 60) % 60;
    tm_info.tm_sec = uptime_sec % 60;

    return (struct wpm_status_state){
        .timeinfo = tm_info,
    };
}

/*
 * 실제 화면에 시각 정보를 표시하는 함수
 * 시, 분, 초 각각 다른 색상으로 표시
 */
static void set_wpm(struct zmk_widget_wpm_status *widget, struct wpm_status_state state)
{
    char buf[3];

    // 시 (파랑)
    snprintf(buf, sizeof(buf), "%02d", state.timeinfo.tm_hour);
    lv_label_set_text(widget->wpm_label, buf);
    lv_obj_set_style_text_color(widget->wpm_label, lv_palette_main(LV_PALETTE_BLUE), 0);

    // 첫 번째 ":" 표시
    lv_obj_t *colon1 = lv_label_create(widget->obj);
    lv_label_set_text(colon1, ":");
    lv_obj_set_style_text_color(colon1, lv_color_white(), 0);
    lv_obj_align_to(colon1, widget->wpm_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    // 분 (초록)
    lv_obj_t *min_label = lv_label_create(widget->obj);
    snprintf(buf, sizeof(buf), "%02d", state.timeinfo.tm_min);
    lv_label_set_text(min_label, buf);
    lv_obj_set_style_text_color(min_label, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align_to(min_label, colon1, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    // 두 번째 ":" 표시
    lv_obj_t *colon2 = lv_label_create(widget->obj);
    lv_label_set_text(colon2, ":");
    lv_obj_set_style_text_color(colon2, lv_color_white(), 0);
    lv_obj_align_to(colon2, min_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    // 초 (빨강)
    lv_obj_t *sec_label = lv_label_create(widget->obj);
    snprintf(buf, sizeof(buf), "%02d", state.timeinfo.tm_sec);
    lv_label_set_text(sec_label, buf);
    lv_obj_set_style_text_color(sec_label, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align_to(sec_label, colon2, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
}

/*
 * 모든 등록된 위젯에 상태를 반영하는 콜백
 */
static void wpm_status_update_cb(struct wpm_status_state state)
{
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_wpm(widget, state);
    }
}

// 이벤트 리스너 매크로
ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            wpm_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

/*
 * 위젯 초기화 함수
 */
int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 77);

    widget->wpm_label = lv_label_create(widget->obj);
    lv_obj_align(widget->wpm_label, LV_ALIGN_TOP_LEFT, 0, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_status_init();
    return 0;
}

/*
 * 위젯 객체 반환 함수
 */
lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget)
{
    return widget->obj;
}
