#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>      // LV_FONT_DECLARE용 포함

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정 함수
static lv_color_t mod_color(uint8_t mods) {
    if (mods & (MOD_LCTL | MOD_RCTL)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LSFT | MOD_RSFT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LALT | MOD_RALT)) return lv_color_hex(0xA8E6CF);  // 민트
    if (mods & (MOD_LGUI | MOD_RGUI)) return lv_color_hex(0x0383E6);  // 윈도우 색
    return lv_color_black(); // 기본 색상
}
//////////////////////////

//////////////////////////
// C 파일 내부용 확장 구조체
struct mod_status_widget_internal {
    struct zmk_widget_mod_status base; // 기존 구조체
    lv_obj_t *caps_label;              // Caps Word 라벨
};
//////////////////////////

//////////////////////////
// 모디 상태 업데이트
static void update_mod_status(struct mod_status_widget_internal *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    char text[32] = "";
    int idx = 0;

    char *syms[4];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL)) syms[n++] = "󰘴"; // Ctrl
    if (mods & (MOD_LSFT | MOD_RSFT)) syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT)) syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI))
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽";
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = "";
#else
        syms[n++] = "󰘳";
#endif

    for (int i = 0; i < n; ++i) {
        if (i > 0)
            idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->base.label, idx ? text : "");
    lv_obj_set_style_text_color(widget->base.label, mod_color(mods), 0);
}
//////////////////////////

//////////////////////////
// Caps Word 상태 표시
static void update_caps_word_status(struct mod_status_widget_internal *widget, bool active) {
    if (active) {
        lv_label_set_text(widget->caps_label, "CW"); // Caps Word 심볼
        lv_obj_set_style_text_color(widget->caps_label, lv_color_hex(0x00FFE5), 0);
    } else {
        lv_label_set_text(widget->caps_label, "");
        lv_obj_set_style_text_color(widget->caps_label, lv_color_hex(0x202020), 0);
    }
}

// ZMK 이벤트 콜백
static void caps_word_event_listener(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    if (!ev) return;

    struct mod_status_widget_internal *widget = k_timer_user_data_get(&mod_status_timer);
    if (!widget) return;

    update_caps_word_status(widget, ev->active);
}
ZMK_SUBSCRIPTION(widget_mod_status_caps_word, zmk_caps_word_state_changed);
//////////////////////////

//////////////////////////
// 모디 상태 타이머 콜백
static void mod_status_timer_cb(struct k_timer *timer) {
    struct mod_status_widget_internal *widget = k_timer_user_data_get(timer);
    if (!widget) return;
    update_mod_status(widget);
}

static struct k_timer mod_status_timer;
//////////////////////////

//////////////////////////
// 위젯 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *base_widget, lv_obj_t *parent)
{
    static struct mod_status_widget_internal *widget;

    widget = k_malloc(sizeof(*widget));
    if (!widget) return -1;

    widget->base = *base_widget;

    // 컨테이너
    widget->base.obj = lv_obj_create(parent);
    lv_obj_set_size(widget->base.obj, 180, 60);

    // 모디 라벨
    widget->base.label = lv_label_create(widget->base.obj);
    lv_obj_align(widget->base.label, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(widget->base.label, "-");
    lv_obj_set_style_text_font(widget->base.label, &NerdFonts_Regular_40, 0);

    // Caps Word 라벨
    widget->caps_label = lv_label_create(widget->base.obj);
    lv_obj_align(widget->caps_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text(widget->caps_label, "");
    lv_obj_set_style_text_font(widget->caps_label, &NerdFonts_Regular_40, 0);

    // 모디 상태 타이머 초기화
    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    // Caps Word 이벤트 리스너 등록
    widget_mod_status_caps_word_init();

    return 0;
}
//////////////////////////

//////////////////////////
// LVGL 객체 반환
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
//////////////////////////
