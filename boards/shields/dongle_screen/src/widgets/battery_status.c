/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h> // LVGL 기본 헤더

#include <zmk/battery.h>
#include <zmk/split/central.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>

#include "battery_status.h"
#include "../brightness.h"

// 커스텀 폰트 선언 (너드20)
LV_FONT_DECLARE(NerdFonts_Regular_20);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
    #define SOURCE_OFFSET 1
#else
    #define SOURCE_OFFSET 0
#endif

#define BATTERY_TEXT_COLOR_HEX 0xFFFFFF
#define BATTERY_WIDTH 90
#define BATTERY_HEIGHT 20
#define CANVAS_WIDTH 118
#define CANVAS_HEIGHT 32

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
};

struct battery_object {
    lv_obj_t *symbol; // 배터리 캔버스
    lv_obj_t *label;  // 배터리 숫자/텍스트
};

static struct battery_object battery_objects[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];
static lv_color_t battery_image_buffer[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET][CANVAS_WIDTH * CANVAS_HEIGHT];
static int8_t last_battery_levels[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET];

// 초기화: 모든 배터리 레벨을 -1로 초기화
static void init_peripheral_tracking(void) {
    for (int i = 0; i < (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET); i++)
        last_battery_levels[i] = -1;
}

// 배터리 색상 (밝음)
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    if (level <= 15) return lv_color_hex(0xFA0D0B);
    if (level <= 30) return lv_color_hex(0xF98300);
    if (level <= 40) return lv_color_hex(0xFFFF00);
    return lv_color_hex(0x00D500);
}

// 배터리 색상 (어두움, 배경용)
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    if (level <= 15) return lv_color_hex(0xB20908);
    if (level <= 30) return lv_color_hex(0xC76A00);
    if (level <= 40) return lv_color_hex(0xB5B500);
    return lv_color_hex(0x04910A);
}

// 배터리 캔버스 그리기
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);
    lv_draw_rect_dsc_t rect_dsc;

    // 외곽 흰색 테두리
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFFFFFF);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 7;
    lv_canvas_draw_rect(canvas, 8, 0, 102, 32, &rect_dsc);

    // +극 돌출부
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0xFFFFFF);
    rect_dsc.bg_opa = LV_OPA_COVER;
    lv_canvas_draw_rect(canvas, 113, 10, 3, 12, &rect_dsc);

    // 오른쪽 둥근 느낌 주기 (검은 점)
    lv_color_t black = lv_color_hex(0x000000);
    lv_canvas_set_px(canvas, 115, 10, black);
    lv_canvas_set_px(canvas, 115, 21, black);

    // 내부 검정 공백
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_hex(0x000000);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 6;
    lv_canvas_draw_rect(canvas, 10, 2, 98, 28, &rect_dsc);

    // 어두운 배경
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = battery_color_dark(level);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.radius = 3;
    lv_canvas_draw_rect(canvas, 14, 6, BATTERY_WIDTH, BATTERY_HEIGHT, &rect_dsc);

    // 밝은 채움
    if (level > 0) {
        uint8_t width = (level > 100 ? 100 : level);
        uint8_t pixel_width = (uint8_t)((BATTERY_WIDTH * width) / 100);
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = battery_color(level);
        rect_dsc.bg_opa = LV_OPA_COVER;
        rect_dsc.radius = 3;
        lv_canvas_draw_rect(canvas, 14, 6, pixel_width, BATTERY_HEIGHT, &rect_dsc);
    }
}

// 배터리 심볼 + 레이블 업데이트
static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    if (state.source >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + SOURCE_OFFSET) return;

    last_battery_levels[state.source] = state.level;

    lv_obj_t *symbol = battery_objects[state.source].symbol;
    lv_obj_t *label = battery_objects[state.source].label;

    draw_battery(symbol, state.level);

    // 🔹 텍스트 색상 설정 (흰색)
    lv_obj_set_style_text_color(label, lv_color_hex(BATTERY_TEXT_COLOR_HEX), 0);

    // 🔹 폰트 적용 (NerdFonts 20)
    lv_obj_set_style_text_font(label, &NerdFonts_Regular_20, 0);

    // 🔹 그림자 스타일 적용 (경계가 명확한 검정 테두리)
    static lv_style_t outline_style;
    lv_style_init(&outline_style);
    lv_style_set_text_shadow_color(&outline_style, lv_color_hex(0x000000)); // 검정 그림자
    lv_style_set_text_shadow_width(&outline_style, 2);  // 두께(경계 명확)
    lv_style_set_text_shadow_ofs_x(&outline_style, 0);  // X 오프셋
    lv_style_set_text_shadow_ofs_y(&outline_style, 0);  // Y 오프셋
    lv_obj_add_style(label, &outline_style, 0);

    // 🔹 레이블 텍스트
    if (state.level < 1) lv_label_set_text(label, "sleep");
    else lv_label_set_text_fmt(label, "%u", state.level);

    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 🔹 캔버스/레이블 표시
    lv_obj_clear_flag(symbol, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(symbol);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label);
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
    return central_battery_status_get_state(eh);
}

// 위젯 업데이트 콜백
void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_symbol(widget->obj, state);
    }
}

// ZMK 이벤트 구독
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
    int canvas_spacing = 46;
    int container_width = 260;

    lv_obj_set_size(widget->obj, container_width, 40);
    lv_obj_align(widget->obj, LV_ALIGN_TOP_MID, 0, 0);

    int total_width = canvas_count * BATTERY_WIDTH + (canvas_count - 1) * canvas_spacing;

    for (int i = 0; i < canvas_count; i++) {
        lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
        lv_canvas_set_buffer(image_canvas, battery_image_buffer[i], CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

        lv_obj_t *battery_label = lv_label_create(image_canvas);
        lv_obj_set_style_text_font(battery_label, &NerdFonts_Regular_20, 0);
        lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);

        int x_offset = i * (BATTERY_WIDTH + canvas_spacing) - total_width / 2 + BATTERY_WIDTH / 2;
        lv_obj_align(image_canvas, LV_ALIGN_CENTER, x_offset, 0);

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
