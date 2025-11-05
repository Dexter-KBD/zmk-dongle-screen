#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/events/caps_word_state_changed.h> // ✅ 이미 이벤트 선언 포함
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// Caps Word 캐스팅 매크로
//////////////////////////
#define as_zmk_caps_word_state_changed(eh) ((const struct zmk_caps_word_state_changed *)(eh))

//////////////////////////
// 모디파이어별 색상 결정 함수
//////////////////////////
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);  // 윈도우색
    return lv_color_black(); // 기본색
}

//////////////////////////
// 전역 상태 구조체
//////////////////////////
struct mod_caps_state {
    bool caps_word_active;
    uint8_t mods;
};

static struct mod_caps_state current_state = {
    .caps_word_active = false,
    .mods = 0,
};

//////////////////////////
// Caps Word 이벤트 콜백
//////////////////////////
static int caps_word_state_listener(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    if (ev) {
        current_state.caps_word_active = ev->active;
        LOG_INF("DISP | Caps Word State Changed: %d", ev->active);
    }
    return 0;
}

ZMK_LISTENER(mod_status_caps_word_listener, caps_word_state_listener);
ZMK_SUBSCRIPTION(mod_status_caps_word_listener, zmk_caps_word_state_changed);

//////////////////////////
// 모드 상태 업데이트
//////////////////////////
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    current_state.mods = mods;

    char text[32] = "";
    int idx = 0;

    char *syms[5];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL))
        syms[n++] = "󰘴";
    if (mods & (MOD_LSFT | MOD_RSFT))
        syms[n++] = "󰘶";
    if (mods & (MOD_LALT | MOD_RALT))
        syms[n++] = "󰘵";
    if (mods & (MOD_LGUI | MOD_RGUI))
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽";
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = "";
#else
        syms[n++] = "󰘳";
#endif

    if (current_state.caps_word_active)
        syms[n++] = "🅰";

    for (int i = 0; i < n; ++i) {
        if (i > 0)
            idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->label, idx ? text : "");
    if (current_state.caps_word_active)
        lv_obj_set_style_text_color(widget->label, lv_color_hex(0x00FFE5), 0);
    else
        lv_obj_set_style_text_color(widget->label, mod_color(mods), 0);
}

//////////////////////////
// 타이머 콜백
//////////////////////////
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

static struct k_timer mod_status_timer;

//////////////////////////
// 초기화
//////////////////////////
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
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
// 객체 반환
//////////////////////////
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
