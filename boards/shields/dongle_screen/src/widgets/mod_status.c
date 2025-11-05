#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h> // LV_FONT_DECLARE용 포함
#include "sf_symbols.h" // Caps Word 심볼 사용

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정 함수
static lv_color_t mod_color(uint8_t mods, bool caps_word) {
    if (caps_word) return lv_color_hex(0x00FFE5);          // Caps Word 활성 색
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);  // 윈도우 색
    return lv_color_black(); // 기본 색상
}
//////////////////////////

//////////////////////////
// 전역 Caps Word 상태
static bool caps_word_active = false;

//////////////////////////
// 모디 상태 업데이트 함수
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers; // 현재 모디 상태 읽기
    char text[32] = ""; // 출력 문자열 버퍼
    int idx = 0;

    // 심볼 임시 배열
    char *syms[5]; // 모디 + Caps Word
    int n = 0;

    // 모디 상태별 심볼 지정
    if (mods & (MOD_LCTL | MOD_RCTL))
        syms[n++] = SF_SYMBOL_CONTROL; // Control
    if (mods & (MOD_LSFT | MOD_RSFT))
        syms[n++] = SF_SYMBOL_SHIFT;   // Shift
    if (mods & (MOD_LALT | MOD_RALT))
        syms[n++] = SF_SYMBOL_ALT;     // Alt
    if (mods & (MOD_LGUI | MOD_RGUI))
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = SF_SYMBOL_SYSTEM_1;
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = SF_SYMBOL_SYSTEM_2;
#else
        syms[n++] = SF_SYMBOL_SYSTEM_DEFAULT;
#endif

    // Caps Word 활성 시 커서 심볼 추가
    if (caps_word_active)
        syms[n++] = SF_SYMBOL_CURSOR_IBEAM; // Caps Word 심볼

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
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}
//////////////////////////

static struct k_timer mod_status_timer;

//////////////////////////
// Caps Word 이벤트 핸들러
static int caps_word_listener(const zmk_event_t *eh)
{
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    caps_word_active = ev->active;
    return 0;
}

// 이벤트 구독
ZMK_SUBSCRIPTION(mod_status_caps_word, zmk_caps_word_state_changed);
ZMK_LISTENER(mod_status_caps_word, caps_word_listener);

//////////////////////////
// 모디 상태 위젯 초기화
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
