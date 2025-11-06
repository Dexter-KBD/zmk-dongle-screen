#include "mod_caps_widget.h"

#include <zmk/display.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/hid.h>

#include <fonts.h>
#include <sf_symbols.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

//////////////////////////
// 모디 색상 결정
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);
    return lv_color_black();
}

//////////////////////////
// Caps Word 색상 결정
static void caps_word_set_active(lv_obj_t *label, bool active) {
    if (active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00FFE5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

//////////////////////////
// 모디 상태 업데이트
static void update_mod_label(struct zmk_widget_mod_caps *widget) {
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    char text[32] = "";
    int idx = 0;

    char *syms[4];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL)) syms[n++] = "󰘴";
    if (mods & (MOD_LSFT | MOD_RSFT)) syms[n++] = "󰘶";
    if (mods & (MOD_LALT | MOD_RALT)) syms[n++] = "󰘵";
    if (mods & (MOD_LGUI | MOD_RGUI)) syms[n++] = ""; // 시스템 아이콘 예시

    for (int i = 0; i < n; ++i) {
        if (i > 0) idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->mod_label, idx ? text : "-");
    lv_obj_set_style_text_color(widget->mod_label, mod_color(mods), 0);
}

//////////////////////////
// Caps Word 상태 업데이트
static void caps_word_update_widgets(bool active) {
    struct zmk_widget_mod_caps *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        caps_word_set_active(widget->caps_label, active);
    }
}

//////////////////////////
// ZMK 이벤트 리스너
static struct caps_word_indicator_state caps_word_get_state(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    return (struct caps_word_indicator_state){ .active = ev->active };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mod_caps, struct caps_word_indicator_state,
                            caps_word_update_widgets, caps_word_get_state)
ZMK_SUBSCRIPTION(widget_mod_caps, zmk_caps_word_state_changed);

//////////////////////////
// 주기적 타이머
static void mod_caps_timer_cb(struct k_timer *timer) {
    struct zmk_widget_mod_caps *widget = k_timer_user_data_get(timer);
    update_mod_label(widget);
}

static struct k_timer mod_caps_timer;

//////////////////////////
// 초기화
int zmk_widget_mod_caps_init(struct zmk_widget_mod_caps *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    widget->mod_label = lv_label_create(widget->obj);
    lv_obj_align(widget->mod_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(widget->mod_label, "-");
    lv_obj_set_style_text_font(widget->mod_label, &NerdFonts_Regular_40, 0);

    widget->caps_label = lv_label_create(widget->obj);
    lv_obj_align(widget->caps_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text(widget->caps_label, SF_SYMBOL_CHARACTER_CURSOR_IBEAM);
    lv_obj_set_style_text_font(widget->caps_label, &SF_Compact_Text_Bold_32, 0);
    caps_word_set_active(widget->caps_label, false);

    sys_slist_append(&widgets, &widget->node);

    // 타이머 초기화
    k_timer_init(&mod_caps_timer, mod_caps_timer_cb, NULL);
    k_timer_user_data_set(&mod_caps_timer, widget);
    k_timer_start(&mod_caps_timer, K_MSEC(100), K_MSEC(100));

    return 0;
}

lv_obj_t *zmk_widget_mod_caps_obj(struct zmk_widget_mod_caps *widget) {
    return widget->obj;
}
