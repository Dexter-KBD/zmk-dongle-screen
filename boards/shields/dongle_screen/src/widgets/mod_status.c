#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <lvgl.h>
#include <fonts.h>      // NerdFont 사용
#include <sf_symbols.h> // SF Symbols 일부 참조

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
// Caps Word 상태 관리 구조체
struct caps_word_state {
    bool active;
};

static sys_slist_t caps_word_widgets = SYS_SLIST_STATIC_INIT(&caps_word_widgets);

static void caps_word_set_active(lv_obj_t *label, struct caps_word_state state) {
    if (state.active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00FFE5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

static void caps_word_update_widgets(struct caps_word_state state) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&caps_word_widgets, widget, node) {
        caps_word_set_active(widget->label, state);
    }
}

//////////////////////////
// 모디 상태 업데이트 함수
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    char text[32] = "";
    int idx = 0;

    char *syms[5];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL))
        syms[n++] = "󰘴"; // Control
    if (mods & (MOD_LSFT | MOD_RSFT))
        syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT))
        syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI))
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽"; // 시스템 1
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = ""; // 시스템 2
#else
        syms[n++] = "󰘳"; // 기본 시스템
#endif

    // Caps Word 활성 상태 심볼 추가
    struct caps_word_state cw_state = { .active = zmk_hid_get_keyboard_report()->body.caps_word_active };
    if (cw_state.active) {
        syms[n++] = ""; // Caps Word 심볼 (SF Symbol)
    }

    for (int i = 0; i < n; ++i) {
        if (i > 0)
            idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->label, idx ? text : "");
    lv_obj_set_style_text_color(widget->label, mod_color(mods), 0);

    // Caps Word 색상 업데이트
    caps_word_update_widgets(cw_state);
}
//////////////////////////

//////////////////////////
// 모디 상태 타이머 콜백
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}
static struct k_timer mod_status_timer;

//////////////////////////
// 모디 상태 위젯 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-");
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

    sys_slist_append(&caps_word_widgets, &widget->node);

    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    return 0;
}

//////////////////////////
// LVGL 객체 반환
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
