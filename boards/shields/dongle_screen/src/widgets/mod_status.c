#include "mod_status.h"

#include <zmk/display.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>

#include <fonts.h>
#include <sf_symbols.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// 위젯 리스트
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// 라벨 색상 업데이트
static void mod_status_set_label_active(lv_obj_t *label, bool active) {
    if (active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00ffe5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

// 전체 위젯 업데이트
static void mod_status_update_widgets(bool caps_active) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        mod_status_set_label_active(widget->caps_label, caps_active);
    }
}

// 이벤트로 상태 가져오기
static bool mod_status_get_state(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev =
        as_zmk_keycode_state_changed(eh);

    // 여기서는 예시로 Caps Lock 상태만 처리
    return ev->state.caps_lock;
}

// ZMK 매크로로 이벤트 리스너 등록
ZMK_DISPLAY_WIDGET_LISTENER(widget_mod_status, bool, mod_status_update_widgets, mod_status_get_state)
ZMK_SUBSCRIPTION(widget_mod_status, zmk_keycode_state_changed);

// 초기화 함수
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);

    // 모드 상태 라벨
    widget->mod_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->mod_label, "MOD");
    lv_obj_set_style_text_color(widget->mod_label, lv_color_hex(0x030303), LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->mod_label, &SF_Compact_Text_Bold_32, LV_PART_MAIN);

    // Caps Word 라벨
    widget->caps_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->caps_label, SF_SYMBOL_CHARACTER_CURSOR_IBEAM);
    lv_obj_set_style_text_color(widget->caps_label, lv_color_hex(0x030303), LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->caps_label, &SF_Compact_Text_Bold_32, LV_PART_MAIN);

    sys_slist_append(&widgets, &widget->node);

    // 위젯 초기화 이벤트 등록
    widget_mod_status_init();

    return 0;
}

// 객체 반환
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->obj;
}
