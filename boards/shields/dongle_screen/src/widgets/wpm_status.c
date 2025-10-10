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
#include <zmk/events/wpm_state_changed.h> // 기존 이벤트 포함
#include "wpm_status.h"                   // 이름 그대로 유지
#include <lvgl.h>
#include <time.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct wpm_status_state
{
    struct tm timeinfo; // 한국 시간
};

// 한국 시간 기준으로 현재 시각 가져오기
static struct wpm_status_state get_state(const zmk_event_t *_eh)
{
    time_t now = k_uptime_seconds(); // Zephyr에서 UPTIME 기준
    struct tm tm_info;

    // k_uptime_seconds()는 epoch가 아니므로 RTC 필요 시 따로 구현
    time_t t = time(NULL) + 9 * 3600; // UTC -> KST(+9시간)
    gmtime_r(&t, &tm_info);

    return (struct wpm_status_state){
        .timeinfo = tm_info
    };
}

static void set_wpm(struct zmk_widget_wpm_status *widget, struct wpm_status_state state)
{
    char buf[3];

    // 시각: 시, 분, 초 각각 다른 색상
    snprintf(buf, sizeof(buf), "%02d", state.timeinfo.tm_hour);
    lv_label_set_text(widget->wpm_label, buf);
    lv_obj_set_style_text_color(widget->wpm_label, lv_palette_main(LV_PALETTE_BLUE), 0);

    // ":" 표시
    lv_obj_t *colon1 = lv_label_create(widget->obj);
    lv_label_set_text(colon1, ":");
    lv_obj_set_style_text_color(colon1, lv_color_white(), 0);
    lv_obj_align_to(colon1, widget->wpm_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    // 분
    lv_obj_t *min_label = lv_label_create(widget->obj);
    snprintf(buf, sizeof(buf), "%02d", state.timeinfo.tm_min);
    lv_label_set_text(min_label, buf);
    lv_obj_set_style_text_color(min_label, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align_to(min_label, colon1, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    // ":" 표시
    lv_obj_t *colon2 = lv_label_create(widget->obj);
    lv_label_set_text(colon2, ":");
    lv_obj_set_style_text_color(colon2, lv_color_white(), 0);
    lv_obj_align_to(colon2, min_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    // 초
    lv_obj_t *sec_label = lv_label_create(widget->obj);
    snprintf(buf, sizeof(buf), "%02d", state.timeinfo.tm_sec);
    lv_label_set_text(sec_label, buf);
    lv_obj_set_style_text_color(sec_label, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align_to(sec_label, colon2, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
}

static void wpm_status_update_cb(struct wpm_status_state state)
{
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        set_wpm(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            wpm_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

// widget init
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

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget)
{
    return widget->obj;
}
