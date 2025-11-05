#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>       // LV_FONT_DECLARE용
#include "sf_symbols.h"  // Caps Word 심볼 정의

#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);  // 윈도우 색
    return lv_color_black(); // 기본
}
//////////////////////////

//////////////////////////
// Caps Word 상태 구조체
struct caps_word_indicator_state {
    bool active;
};

static struct caps_word_indicator_state caps_word_indicator = {
    .active = false,
};

static void caps_word_set_active(struct zmk_widget_mod_status *widget, bool active) {
    caps_word_indicator.active = active;
    if (active) {
        lv_label_set_text(widget->caps_label, SF_SYMBOL_CURSOR_IBEAM);
        lv_obj_set_style_text_color(widget->caps_label, lv_color_hex(0x00FFE5), 0);
    } else {
        lv_label_set_text(widget->caps_label, "");
    }
}

// Caps Word 이벤트 콜백
static void caps_word_listener(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev =
        as_zmk_caps_word_state_changed(eh);
    caps_word_set_active((struct zmk_widget_mod_status *)ev->user_data, ev->active);
}
ZMK_SUBSCRIPTION(mod_status_caps_word_listener, zmk_caps_word_state_changed);

//////////////////////////
// 모디 상태 업데이트
static void update_mod_status(struct zmk_widget_mod_status *widget) {
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;

    char text[32] = "";
    int idx = 0;
    char *syms[4];
    int n = 0;

    // 기존 모디파이어 심볼
    if (mods & (MOD_LCTL | MOD_RCTL)) syms[n++] = "󰘴"; // Control
    if (mods & (MOD_LSFT | MOD_RSFT)) syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT)) syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI)) syms[n++] = "󰘳"; // GUI

    // 심볼들을 공백으로 연결
    for (int i = 0; i < n; ++i) {
        if (i > 0) idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->label, idx ? text : "");
    lv_obj_set_style_text_color(widget->label, mod_color(mods), 0);
}
//////////////////////////

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
    // LVGL 컨테이너
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    // LVGL 모디 라벨
    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-");
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

    // LVGL Caps Word 라벨
    widget->caps_label = lv_label_create(widget->obj);
    lv_obj_align(widget->caps_label, LV_ALIGN_CENTER, 0, -10);
    lv_label_set_text(widget->caps_label, "");
    lv_obj_set_style_text_font(widget->caps_label, &NerdFonts_Regular_40, 0);

    // 타이머 설정
    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    // Caps Word 이벤트 구독
    zmk_event_manager_subscribe(zmk_caps_word_state_changed, (void*)widget);

    return 0;
}
//////////////////////////

//////////////////////////
// LVGL 객체 반환
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->obj;
}
