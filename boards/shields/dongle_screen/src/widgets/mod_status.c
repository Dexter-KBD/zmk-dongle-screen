#include "mod_status.h"
#include <zmk/display.h>
#include <zmk/events/keyboard_state_changed.h>
#include <zmk/event_manager.h>

#include <fonts.h>
#include <sf_symbols.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// ====== 상태 업데이트 ======
struct mod_status_state {
    bool caps_active;
    bool shift_active;
    bool ctrl_active;
};

static void mod_status_set_label_color(lv_obj_t *label, bool active) {
    lv_color_t color = active ? lv_color_hex(0x00ffe5) : lv_color_hex(0x202020);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
}

static void mod_status_update_widgets(struct mod_status_state state) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        mod_status_set_label_color(widget->caps_label, state.caps_active);
        mod_status_set_label_color(widget->mod_label, state.shift_active || state.ctrl_active);
    }
}

// ====== 이벤트 변환 ======
static struct mod_status_state mod_status_get_state(const zmk_event_t *eh) {
    const struct zmk_keyboard_state_changed *ev = as_zmk_keyboard_state_changed(eh);
    return (struct mod_status_state){
        .caps_active = ev->mods & MOD_MASK_CAPS,
        .shift_active = ev->mods & (MOD_MASK_SHIFT_L | MOD_MASK_SHIFT_R),
        .ctrl_active = ev->mods & (MOD_MASK_CTRL_L | MOD_MASK_CTRL_R),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mod_status, struct mod_status_state,
                            mod_status_update_widgets, mod_status_get_state)
ZMK_SUBSCRIPTION(widget_mod_status, zmk_keyboard_state_changed);

// ====== 초기화 ======
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    widget->mod_label = lv_label_create(parent);
    lv_label_set_text(widget->mod_label, SF_SYMBOL_ARROW_UP); // Shift/Control 대표 심볼
    lv_obj_set_style_text_color(widget->mod_label, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->mod_label, &SF_Compact_Text_Bold_32, LV_PART_MAIN);
    lv_obj_set_style_text_align(widget->mod_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(widget->mod_label, LV_ALIGN_CENTER, -20, 0); // 위치 예시

    widget->caps_label = lv_label_create(parent);
    lv_label_set_text(widget->caps_label, SF_SYMBOL_CHARACTER_CURSOR_IBEAM);
    lv_obj_set_style_text_color(widget->caps_label, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->caps_label, &SF_Compact_Text_Bold_32, LV_PART_MAIN);
    lv_obj_set_style_text_align(widget->caps_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(widget->caps_label, LV_ALIGN_CENTER, 20, 0); // 위치 예시

    sys_slist_append(&widgets, &widget->node);

    // 초기 업데이트
    struct mod_status_state initial = {0};
    mod_status_update_widgets(initial);

    return 0;
}

lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->mod_label; // 대표 객체 반환
}
