#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include <zmk/display.h>
#include <zmk/events/caps_word_state_changed.h> // ✅ 이벤트 기반 Caps Word 감지용
#include <zmk/event_manager.h>
#include <lvgl.h>
#include "mod_status.h"
#include <fonts.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

//////////////////////////
// 모디파이어별 색상 결정 함수
// Caps Word가 활성화되면 노란색으로 표시
static lv_color_t mod_color(uint8_t mods, bool caps_word) {
    if (caps_word)
        return lv_color_hex(0xFFD54F); // Caps Word 활성 시 노란색
    if (mods & (MOD_LCTL | MOD_RCTL))
        return lv_color_hex(0xA8E6CF); // 민트
    if (mods & (MOD_LSFT | MOD_RSFT))
        return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LALT | MOD_RALT))
        return lv_color_hex(0xA8E6CF);
    if (mods & (MOD_LGUI | MOD_RGUI))
        return lv_color_hex(0x0383E6); // 윈도우 키 색
    return lv_color_black();
}

//////////////////////////
// 위젯 상태 구조체
struct zmk_widget_mod_status {
    lv_obj_t *obj;
    lv_obj_t *label;
    bool caps_word_active; // ✅ Caps Word 상태 저장
};

//////////////////////////
// 모디 상태 업데이트 함수
static void update_mod_status(struct zmk_widget_mod_status *widget)
{
    uint8_t mods = zmk_hid_get_keyboard_report()->body.modifiers;
    bool caps_word = widget->caps_word_active; // ✅ 이벤트로 업데이트된 상태 사용

    char text[48] = "";
    int idx = 0;
    char *syms[5];
    int n = 0;

    if (mods & (MOD_LCTL | MOD_RCTL))
        syms[n++] = "󰘴"; // Ctrl
    if (mods & (MOD_LSFT | MOD_RSFT))
        syms[n++] = "󰘶"; // Shift
    if (mods & (MOD_LALT | MOD_RALT))
        syms[n++] = "󰘵"; // Alt
    if (mods & (MOD_LGUI | MOD_RGUI)) {
#if CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 1
        syms[n++] = "󰌽";
#elif CONFIG_DONGLE_SCREEN_SYSTEM_ICON == 2
        syms[n++] = "";
#else
        syms[n++] = "󰘳";
#endif
    }

    // ✅ Caps Word 활성 시 심볼 추가
    if (caps_word)
        syms[n++] = "󰘲"; // Caps Word 아이콘

    for (int i = 0; i < n; ++i) {
        if (i > 0)
            idx += snprintf(&text[idx], sizeof(text) - idx, " ");
        idx += snprintf(&text[idx], sizeof(text) - idx, "%s", syms[i]);
    }

    lv_label_set_text(widget->label, idx ? text : "");
    lv_obj_set_style_text_color(widget->label, mod_color(mods, caps_word), 0);
}

//////////////////////////
// 타이머 콜백 (모디 상태 주기 갱신)
static void mod_status_timer_cb(struct k_timer *timer)
{
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

static struct k_timer mod_status_timer;

//////////////////////////
// Caps Word 상태 이벤트 수신 콜백
// (ZMK가 caps_word_state_changed 이벤트를 발행할 때 호출됨)
static int caps_word_state_changed_listener(const zmk_event_t *eh)
{
    const struct zmk_caps_word_state_changed *ev = as_zmk_caps_word_state_changed(eh);
    if (!ev) return 0;

    LOG_INF("DISP | Caps Word changed: %d", ev->active);

    extern struct zmk_widget_mod_status global_mod_widget; // ✅ 전역 참조
    global_mod_widget.caps_word_active = ev->active;

    // 상태 즉시 갱신
    update_mod_status(&global_mod_widget);
    return 0;
}

ZMK_LISTENER(mod_status, caps_word_state_changed_listener);
ZMK_SUBSCRIPTION(mod_status, zmk_caps_word_state_changed);

//////////////////////////
// 위젯 초기화
struct zmk_widget_mod_status global_mod_widget; // ✅ 전역 위젯 인스턴스

int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 180, 40);

    widget->label = lv_label_create(widget->obj);
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_text(widget->label, "-");
    lv_obj_set_style_text_font(widget->label, &NerdFonts_Regular_40, 0);

    widget->caps_word_active = false; // 초기 상태 비활성화

    // 타이머 시작 (모디 상태 주기 갱신)
    k_timer_init(&mod_status_timer, mod_status_timer_cb, NULL);
    k_timer_user_data_set(&mod_status_timer, widget);
    k_timer_start(&mod_status_timer, K_MSEC(100), K_MSEC(100));

    // 전역 포인터로 등록
    global_mod_widget = *widget;

    return 0;
}

lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget)
{
    return widget->obj;
}
