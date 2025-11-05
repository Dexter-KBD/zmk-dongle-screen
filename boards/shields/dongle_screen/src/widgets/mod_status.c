#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/caps_word_state_changed.h>
#include <fonts.h>
#include <sf_symbols.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// 기존 모디 상태용 linked list
static sys_slist_t mod_widgets = SYS_SLIST_STATIC_INIT(&mod_widgets);

// 기존 구조체
struct zmk_widget_mod_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *shift_label;
    lv_obj_t *ctrl_label;
    lv_obj_t *alt_label;
    lv_obj_t *gui_label;
    lv_obj_t *caps_label; // 새로 추가한 Caps Word 레이블
};

// 기존 색상 및 업데이트 함수 유지
static void update_mod_status(struct zmk_widget_mod_status *widget) {
    // Shift
    if (widget->shift_label) {
        lv_label_set_text(widget->shift_label,
            zmk_keycode_state_shift_active() ? SF_SYMBOL_CHARACTER_SHIFT : SF_SYMBOL_CHARACTER_NONE);
    }

    // Ctrl
    if (widget->ctrl_label) {
        lv_label_set_text(widget->ctrl_label,
            zmk_keycode_state_ctrl_active() ? SF_SYMBOL_CHARACTER_CTRL : SF_SYMBOL_CHARACTER_NONE);
    }

    // Alt
    if (widget->alt_label) {
        lv_label_set_text(widget->alt_label,
            zmk_keycode_state_alt_active() ? SF_SYMBOL_CHARACTER_ALT : SF_SYMBOL_CHARACTER_NONE);
    }

    // GUI
    if (widget->gui_label) {
        lv_label_set_text(widget->gui_label,
            zmk_keycode_state_gui_active() ? SF_SYMBOL_CHARACTER_GUI : SF_SYMBOL_CHARACTER_NONE);
    }

    // Caps Word
    if (widget->caps_label) {
        lv_label_set_text(widget->caps_label,
            zmk_caps_word_active() ? SF_SYMBOL_CHARACTER_CURSOR_IBEAM : SF_SYMBOL_CHARACTER_NONE);
    }
}

// Caps Word 이벤트 콜백
static void caps_word_event_listener(const zmk_event_t *eh) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&mod_widgets, widget, node) {
        update_mod_status(widget);
    }
}

// 모디 상태 이벤트 콜백
static void mod_state_event_listener(const zmk_event_t *eh) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&mod_widgets, widget, node) {
        update_mod_status(widget);
    }
}

// 위젯 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);

    // Shift Label
    widget->shift_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->shift_label, SF_SYMBOL_CHARACTER_NONE);

    // Ctrl Label
    widget->ctrl_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->ctrl_label, SF_SYMBOL_CHARACTER_NONE);

    // Alt Label
    widget->alt_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->alt_label, SF_SYMBOL_CHARACTER_NONE);

    // GUI Label
    widget->gui_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->gui_label, SF_SYMBOL_CHARACTER_NONE);

    // Caps Word Label
    widget->caps_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->caps_label, SF_SYMBOL_CHARACTER_NONE);
    lv_obj_set_style_text_color(widget->caps_label, lv_color_hex(0x00ffe5), LV_PART_MAIN);

    // linked list에 추가
    sys_slist_append(&mod_widgets, &widget->node);

    // 초기 상태 업데이트
    update_mod_status(widget);

    return 0;
}

// 위젯 객체 가져오기
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->obj;
}

// 이벤트 구독
ZMK_SUBSCRIPTION(widget_mod_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_mod_status, zmk_caps_word_state_changed);

