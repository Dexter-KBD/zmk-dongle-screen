#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>
#include <sf_symbols.h> // Caps Word 심볼용

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);  // 윈도우 색
    return lv_color_black(); // 기본 색상
}

//////////////////////////
// 현재 Caps Word 상태 반환
static bool caps_word_active(void) {
    // 항상 y로 포함되도록, 외부 이벤트 없이도 상태 가져오기
    // 실제로 이벤트 기반으로 바꾸려면 zmk_caps_word_state_changed 이벤트 사용
    extern bool zmk_caps_word_state; // 추가한 caps_word_state_changed.c에서 정의
    return zmk_caps_word_state;
}

//////////////////////////
// 모디 상태 + Caps Word 업데이트
static void update_mod_status(struct zmk_widget_mod_status *widget) {
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    char text[32] = "";
    int idx = 0;

    // 모디 심볼 배열
    char *syms[4];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL)) syms[n++] = "󰘴"; // Ctrl
    if (mods & (MOD_LSFT | MOD_RSFT)) syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT)) syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI)) syms[n++] = "󰘳"; // GUI

    // Caps Word 활성 시 커서 심볼 추가
    if (caps_word_active()) {
        if (n > 0) idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, SF_SYMBOL_CURSOR_IBEAM);
    }

    // 모디 심볼들을 공백으로 이어붙임
    for (int i = 0; i < n; ++i) {
        if (i > 0) idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    // LVGL 라벨에 텍스트 적용
    lv_label_set_text(widget->label, idx ? text : "-");

    // 색상 적용
    lv_obj_set_style_text_color(widget->label, mod_color(mods), 0);
}

//////////////////////////
// 주기적 업데이트 타이머
static void mod_status_timer_cb(struct k_timer *timer) {
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

static struct k_timer mod_status_timer;

//////////////////////////
// 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-"); 
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

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
