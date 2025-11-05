#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/caps_word.h> // ✅ Caps Word 감지용 헤더
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h> // LV_FONT_DECLARE용 포함

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정 함수
// Caps Word가 활성화되면 우선순위로 색상을 노란색으로 표시
static lv_color_t mod_color(uint8_t mods, bool caps_word) {
    if (caps_word)
        return lv_color_hex(0xFFD54F); // Caps Word 활성 시 노란색
    if (mods & (MOD_LCTL | MOD_RCTL))
        return lv_color_hex(0xA8E6CF); // 민트
    if (mods & (MOD_LSFT | MOD_RSFT))
        return lv_color_hex(0xA8E6CF); // 민트
    if (mods & (MOD_LALT | MOD_RALT))
        return lv_color_hex(0xA8E6CF); // 민트
    if (mods & (MOD_LGUI | MOD_RGUI))
        return lv_color_hex(0x0383E6); // 윈도우 색
    return lv_color_black(); // 기본 색상
}
//////////////////////////

//////////////////////////
// 모디 상태 업데이트 함수
// HID 보고서로 모디 상태를 읽고, Caps Word 활성 여부를 추가로 확인
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers; // 현재 모디 상태
    bool caps_word = zmk_caps_word_active();                      // ✅ Caps Word 활성 상태 읽기

    char text[48] = "";
    int idx = 0;
    char *syms[5];
    int n = 0;

    // 모디 상태별 심볼 지정
    if (mods & (MOD_LCTL | MOD_RCTL))
        syms[n++] = "󰘴"; // Control
    if (mods & (MOD_LSFT | MOD_RSFT))
        syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT))
        syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI)) {
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽"; // 시스템 1
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = ""; // 시스템 2
#else
        syms[n++] = "󰘳"; // 기본 시스템
#endif
    }

    // ✅ Caps Word 활성 시 심볼 추가
    if (caps_word)
        syms[n++] = "󰘲"; // Caps Word 아이콘 (Nerd Font 기반)

    // 심볼들을 공백으로 구분하여 text에 합치기
    for (int i = 0; i < n; ++i) {
        if (i > 0)
            idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    // LVGL 라벨에 텍스트 적용
    lv_label_set_text(widget->label, idx ? text : "");

    // 색상 적용 (Caps Word일 경우 우선)
    lv_obj_set_style_text_color(widget->label, mod_color(mods, caps_word), 0);
}
//////////////////////////

//////////////////////////
// 타이머 콜백 함수
// 100ms 주기로 update_mod_status를 호출
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}
//////////////////////////

static struct k_timer mod_status_timer;

//////////////////////////
// 모디 상태 위젯 초기화 함수
// LVGL 객체 생성 후 주기적 갱신 타이머 시작
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    // 컨테이너 객체 생성
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    // 라벨 객체 생성
    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-");
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

    // 타이머 설정 및 시작
    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    return 0;
}
//////////////////////////

//////////////////////////
// LVGL 객체 반환 함수
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
//////////////////////////
