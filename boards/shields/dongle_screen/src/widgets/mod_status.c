#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/caps_word.h> // ✅ Caps Word 상태 확인용
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h> // LV_FONT_DECLARE용 포함

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정 함수
// 각 키보드 모디 상태에 따라 텍스트 색상을 반환
static lv_color_t mod_color(uint8_t mods, bool caps_word_active) {
    // Caps Word가 활성화되면 민트색 우선
    if (caps_word_active) return lv_color_hex(0x00FFE5);

    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);  // 윈도우 색
    return lv_color_black(); // 기본 색상
}
//////////////////////////

//////////////////////////
// 모디 상태 업데이트 함수
// 키보드 HID 레포트를 읽어 현재 모디 상태를 심볼과 색상으로 갱신
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    const struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    uint8_t mods = report ? report->body.modifiers : 0;

    bool caps_word_active = zmk_caps_word_get_state(); // ✅ Caps Word 상태 가져오기

    char text[48] = ""; // 출력 문자열 버퍼
    int idx = 0;

    // 심볼 임시 배열
    const char *syms[5];
    int n = 0;

    // 모디 상태별 심볼 지정
    if (mods & (MOD_LCTL | MOD_RCTL))
        syms[n++] = "󰘴"; // Control
    if (mods & (MOD_LSFT | MOD_RSFT))
        syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT))
        syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI))
    // 시스템 아이콘 설정에 따른 심볼
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽"; // 시스템 1
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = ""; // 시스템 2
#else
        syms[n++] = "󰘳"; // 기본 시스템
#endif

    // ✅ Caps Word 활성 시 🅰 추가
    if (caps_word_active)
        syms[n++] = "🅰";

    // 심볼들을 공백으로 구분하여 text 배열에 복사
    for (int i = 0; i < n; ++i) {
        if (i > 0)
            idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    // LVGL 라벨에 텍스트 적용
    lv_label_set_text(widget->label, idx ? text : "-");

    // LVGL 라벨에 색상 적용
    lv_obj_set_style_text_color(widget->label, mod_color(mods, caps_word_active), 0);
}
//////////////////////////

//////////////////////////
// 모디 상태 타이머 콜백
// 주기적으로 update_mod_status 호출
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}
//////////////////////////

static struct k_timer mod_status_timer;

//////////////////////////
// 모디 상태 위젯 초기화
// parent 객체에 LVGL 객체 생성 후 타이머 시작
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    // LVGL 컨테이너 객체 생성
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    // LVGL 라벨 객체 생성
    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-"); // 초기 텍스트
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0); // NerdFont 설정

    // 타이머 초기화 및 주기적 업데이트
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
