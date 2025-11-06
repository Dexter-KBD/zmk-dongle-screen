#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>
#include <lvgl.h>
#include <fonts.h>       // NerdFont
#include <sf_symbols.h>  // SF Symbols

#include "mod_status.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 구조체: 모디 + Caps Word 상태 관리
struct zmk_widget_mod_status {
    sys_snode_t node;   // 리스트 연결용
    lv_obj_t *obj;      // LVGL 컨테이너
    lv_obj_t *label;    // LVGL 라벨
};

static sys_slist_t mod_status_widgets = SYS_SLIST_STATIC_INIT(&mod_status_widgets);

//////////////////////////
// 모디별 색상 결정
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);
    return lv_color_black();
}

//////////////////////////
// Caps Word 상태 구조체
struct caps_word_state {
    bool active;
};

// Caps Word 라벨 색상 업데이트
static void caps_word_set_active(lv_obj_t *label, struct caps_word_state state) {
    if (state.active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00ffe5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

// 모든 위젯 갱신
static void caps_word_update_widgets(struct caps_word_state state) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&mod_status_widgets, widget, node) {
        caps_word_set_active(widget->label, state);
    }
}

//////////////////////////
// Caps Word 이벤트 콜백
static struct caps_word_state caps_word_get_state(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev =
        as_zmk_caps_word_state_changed(eh);
    return (struct caps_word_state){ .active = ev->active };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_caps_word_listener,
                            struct caps_word_state,
                            caps_word_update_widgets,
                            caps_word_get_state)
ZMK_SUBSCRIPTION(widget_caps_word_listener, zmk_caps_word_state_changed);

//////////////////////////
// 모디 + Caps Word 상태 업데이트
static void update_mod_status(struct zmk_widget_mod_status *widget) {
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;

    char text[32] = "";
    int idx = 0;
    char *syms[5];
    int n = 0;

    // 모디별 심볼
    if (mods & (MOD_LCTL | MOD_RCTL)) syms[n++] = "󰘴";
    if (mods & (MOD_LSFT | MOD_RSFT)) syms[n++] = "󰘶";
    if (mods & (MOD_LALT | MOD_RALT)) syms[n++] = "󰘵";
    if (mods & (MOD_LGUI | MOD_RGUI)) syms[n++] = "󰌽";

    for (int i = 0; i < n; ++i) {
        if (i > 0) idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->label, idx ? text : "");
    lv_obj_set_style_text_color(widget->label, mod_color(mods), 0);
}

//////////////////////////
// 타이머 콜백
static void mod_status_timer_cb(struct k_timer *timer) {
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

static struct k_timer mod_status_timer;

//////////////////////////
// 위젯 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-");
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

    sys_slist_append(&mod_status_widgets, &widget->node);

    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    return 0;
}

//////////////////////////
// LVGL 객체 반환
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->obj;
}
