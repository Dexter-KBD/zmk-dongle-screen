/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/split/central.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>

#include "battery_status.h"
#include "../brightness.h"

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
    #define SOURCE_OFFSET 1
#else
    #define SOURCE_OFFSET 0
#endif

#define BATTERY_TEXT_COLOR_HEX 0xFFFFFF
#define BATTERY_WIDTH 90
#define BATTERY_HEIGHT 20
#define BATTERY_SPACING 140
#define CANVAS_WIDTH 112
#define CANVAS_HEIGHT 32

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
};

struct battery_object {
    lv_obj_t *symbol; // 배터리 캔버스
    lv_obj_t *label;  // 배터리 숫자/텍스트
} battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 캔버스 버퍼
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][CANVAS_WIDTH * CANVAS_HEIGHT];

// 마지막 배터리 레벨 저장 (화면 갱신 최소화용)
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 초기화: 모든 배터리 레벨을 -1로 초기화
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++)
        last_battery_levels[i] = -1;
}

// 배터리 색상 (밝음)
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x00E800);
}

// 배터리 색상 (어두움, 배경용)
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xB20908);
    else if (level <= 30) return lv_color_hex(0xC76A00);
    else if (level <= 40) return lv_color_hex(0xB5B500);
    else return lv_color_hex(0x04910A);
}

// 배터리 캔버스 그리기 (건전지 모양)
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    // 캔버스 초기화 (투명 배경)
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
    lv_draw_rect_dsc_t rect_dsc;

    /* ───────────────────────────────
     * 1. 배터리 외곽 (흰색 본체 테두리)
     * ─────────────────────────────── */
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFFFFFF);  // 흰색 테두리
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.border_width = 0;
    rect_dsc.radius = 7;  // 모서리 약간 둥글게
    // (x=2, y=0, width=102, height=32)
    lv_canvas_draw_rect(canvas, 2, 0, 102, 32, &rect_dsc);

    /* ───────────────────────────────
     * 1-2. +극 돌출부 (건전지 머리 부분)
     * ─────────────────────────────── */
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFFFFFF);  // 본체와 같은 흰색
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.border_width = 0;
    rect_dsc.radius = 2;  // 살짝만 둥글게
    // 본체 오른쪽 중앙에 폭 4, 높이 8짜리 돌출부 추가
    // (x=106, y=12)은 대략 본체 중앙 높이 기준으로 계산됨
    lv_canvas_draw_rect(canvas, 106, 12, 4, 8, &rect_dsc);

    /* ───────────────────────────────
     * 2. 내부 검정 공백 (흰색 틀 안쪽 공간)
     * ─────────────────────────────── */
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0x000000);  // 검정색 내부
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 6;
    lv_canvas_draw_rect(canvas, 4, 2, 98, 28, &rect_dsc);

    /* ───────────────────────────────
     * 3. 어두운 배경 (레벨 채움 전 바탕)
     * ─────────────────────────────── */
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = battery_color_dark(level);  // 배터리 단계별 어두운 색
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 3;
    // 실제 배터리 본체 내부 영역
    lv_canvas_draw_rect(canvas, 8, 6, BATTERY_WIDTH, BATTERY_HEIGHT, &rect_dsc);

    /* ───────────────────────────────
     * 4. 밝은 채움 (실제 충전량)
     * ─────────────────────────────── */
    if (level > 0) {
        uint8_t width = (level > 100 ? 100 : level);
        uint8_t pixel_width = (uint8_t)((BATTERY_WIDTH * width) / 100);

        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = battery_color(level);  // 단계별 밝은 색상
        rect_dsc.bg_opa = LV_OPA_COVER;
        rect_dsc.radius = 3;
        lv_canvas_draw_rect(canvas, 8, 6, pixel_width, BATTERY_HEIGHT, &rect_dsc);
    }
}


// 배터리 심볼 + 레이블 업데이트
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    last_battery_levels[state.source] = state.level;

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    lv_obj_set_style_text_color(label, lv_color_hex(BATTERY_TEXT_COLOR_HEX), 0);

    if (state.level < 1) lv_label_set_text(label, "sleep");
    else lv_label_set_text_fmt(label, "%u", state.level);

    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(symbol);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);
}

// 배터리 상태 업데이트 콜백
void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_symbol(widget->obj, state);
    }
}

// 이벤트에서 배터리 상태 가져오기 (Peripheral)
static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){ .source = ev->source + SOURCE_OFFSET, .level = ev->state_of_charge };
}

// 이벤트에서 배터리 상태 가져오기 (Central)
static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){ .source = 0, .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge() };
}

// 이벤트 기반 배터리 상태 결정
static struct battery_state battery_status_get_state(const zmk_event_t *eh) {
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL)
        return peripheral_battery_status_get_state(eh);
    else
        return central_battery_status_get_state(eh);
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#endif
#endif

// 위젯 초기화
int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);

    int canvas_count = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET;
    int canvas_spacing = 46; // ⚠️ 두 캔버스 중앙 기준 간격 50픽셀

    // ⚠️ 컨테이너 폭을 수동으로 260px로 설정
    int container_width = 260;
    lv_obj_set_size(widget->obj, container_width, 40);

    // 화면 상단 중앙에 배치
    lv_obj_align(widget->obj, LV_ALIGN_TOP_MID, 0, 0);

    // 전체 캔버스 폭 계산 (중앙 기준 배치용)
    int total_width = canvas_count * BATTERY_WIDTH + (canvas_count - 1) * canvas_spacing;

    for (int i = 0; i < canvas_count; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

        lv_obj_t *battery_label = lv_label_create(image_canvas);

        // ⚠️ 캔버스 배치: 컨테이너 중앙 기준으로 좌우 균형
        int x_offset = i * (BATTERY_WIDTH + canvas_spacing) - total_width / 2 + BATTERY_WIDTH / 2;
        lv_obj_align(image_canvas, LV_ALIGN_CENTER, x_offset, 0);

        // 레이블은 캔버스 중앙에 배치
        lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_flag(image_canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);

        battery_objects[i] = (struct battery_object){ .symbol = image_canvas, .label = battery_label };
    }

    sys_slist_append(&widgets, &widget->node);
    init_peripheral_tracking();
    widget_dongle_battery_status_init();

    return 0;
}


// 위젯 객체 반환
lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}
