#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h> // LV_FONT_DECLARE용 포함

#include <zmk/display.h>
#include <zmk/events/caps_word_state_changed.h>
#include <zmk/event_manager.h>
#include <sf_symbols.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// -------------------------
// 슬링크드 리스트 초기화
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(widgets);

// -------------------------
// 구조체 정의 (C 파일 내)
struct caps_word_indicator_state {
    bool active;
};

struct zmk_widget_caps_word_indicator {
    lv_obj_t *obj;
    struct sys_snode_t node;
};

// -------------------------
// 모디파이어별 색상 결정 함수
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
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

// -------------------------
// Caps Word Indicator 관련
static void caps_word_indicator_set_active(lv_obj_t *label, struct caps_word_indicator_state state) {
    if (state.active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00ffe5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

static void caps_word_indicator_update_cb(struct caps_word_indicator_state state) {
    struct zmk_widget_caps_word_indicator *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        caps_word_indicator_set_active(widget->obj, state);
    }
}

static struct caps_word_indicator_state caps_word_indicator_get_state(const zmk_event_t *eh) {
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    LOG_INF("DISP | Caps Word State Changed: %d", ev->active);
    return (struct caps_word_indicator_state){ .active = ev->active };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_caps_word_indicator, struct caps_word_indicator_state,
                            caps_word_indicator_update_cb, caps_word_indicator_get_state)
ZMK_SUBSCRIPTION(widget_caps_word_indicator, zmk_caps_word_state_changed);

int zmk_widget_caps_word_indicator_init(struct zmk_widget_caps_word_indicator *widget,
                                        lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    // LV_FONT_DECLARE(SF_Compact_Text_Bold_32); // 필요시 활성화

    lv_label_set_text(widget->obj, SF_SYMBOL_CHARACTER_CURSOR_IBEAM);
    lv_obj_set_style_text_color(widget->obj, lv_color_hex(0x030303), LV_PART_MAIN);
    lv_obj_set_style_text_font(widget->obj, &SF_Compact_Text_Bold_32, LV_PART_MAIN);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    sys_slist_append(&widgets, &widget->node);

    return 0;
}

// -------------------------
static struct k_timer mod_status_timer;

// 모디 상태 위젯 초기화
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

// -------------------------
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
