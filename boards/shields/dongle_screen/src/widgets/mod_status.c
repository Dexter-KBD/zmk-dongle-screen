#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>
#include <zmk/display.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>
#include <sf_symbols.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// -------------------------
// Caps Word 상태 구조체
struct caps_word_indicator_state {
    bool active;
};

// -------------------------
// 모디 색상 결정
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);
    return lv_color_black();
}

// -------------------------
// 모디 상태 업데이트
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    char text[32] = "";
    int idx = 0;

    char *syms[4];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL)) syms[n++] = "󰘴";
    if (mods & (MOD_LSFT | MOD_RSFT)) syms[n++] = "󰘶";
    if (mods & (MOD_LALT | MOD_RALT)) syms[n++] = "󰘵";
    if (mods & (MOD_LGUI | MOD_RGUI))
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽";
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = "";
#else
        syms[n++] = "󰘳";
#endif

    for (int i = 0; i < n; ++i) {
        if (i > 0) idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->label, idx ? text : "");
    lv_obj_set_style_text_color(widget->label, mod_color(mods), 0);
}

// -------------------------
// 모디 상태 타이머 콜백
static struct k_timer mod_status_timer;

static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

// -------------------------
// Caps Word 업데이트
static void caps_word_indicator_set_active(lv_obj_t *label, struct caps_word_indicator_state state) {
    if (state.active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00ffe5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

static struct caps_word_indicator_state caps_word_indicator_get_state(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    LOG_INF("DISP | Caps Word State Changed: %d", ev->active);
    return (struct caps_word_indicator_state){ .active = ev->active };
}

// 단일 위젯용 전역 포인터
static struct zmk_widget_mod_status *mod_status_widget_instance = NULL;

static void caps_word_indicator_update_cb(struct caps_word_indicator_state state) {
    if (!mod_status_widget_instance) return;
    caps_word_indicator_set_active(mod_status_widget_instance->caps_word_label, state);
}

// -------------------------
ZMK_DISPLAY_WIDGET_LISTENER(widget_caps_word_indicator,
                            struct caps_word_indicator_state,
                            caps_word_indicator_update_cb,
                            caps_word_indicator_get_state)
ZMK_SUBSCRIPTION(widget_caps_word_indicator, zmk_caps_word_state_changed);

// -------------------------
// 모디 상태 위젯 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    // 모디 레이블
    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-");
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

    // Caps Word 레이블
    widget->caps_word_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->caps_word_label, "🅰");
    lv_obj_align(widget->caps_word_label, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_set_style_text_color(widget->caps_word_label, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->caps_word_label, &NerdFonts_Regular_40, LV_PART_MAIN);

    // 전역 포인터 설정
    mod_status_widget_instance = widget;

    // 모디 상태 타이머
    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    return 0;
}

// -------------------------
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
