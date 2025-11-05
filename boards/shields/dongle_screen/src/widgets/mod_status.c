#include <zephyr/kernel.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/caps_word.h> // ✅ ZMK에 기본 내장된 Caps Word 상태 함수
#include <zmk/display.h>

#include <lvgl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "mod_status.h"

// ---------------------------------------------------------
// 🧩 구조체 정의
// ---------------------------------------------------------
struct mod_status_state {
    bool mods_active;
    bool caps_word_active; // ✅ Caps Word 상태 추가
};

// ---------------------------------------------------------
// 🎨 스타일 함수
// ---------------------------------------------------------
static void set_label_color(lv_obj_t *label, bool active) {
    if (active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00FFE5), LV_PART_MAIN); // 활성 시 하늘색
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x303030), LV_PART_MAIN); // 비활성 시 어두운 회색
    }
}

// ---------------------------------------------------------
// 💡 상태 갱신 함수
// ---------------------------------------------------------
static void update_mod_status(struct zmk_widget_mod_status *widget) {
    // 모디파이어 상태 확인
    uint8_t mods = zmk_keymap_mods();
    bool mods_active = mods != 0;

    // Caps Word 상태 확인 (✅ 폴링 방식)
    bool caps_word_active = zmk_caps_word_get_state();

    // 상태 변화 확인
    static struct mod_status_state last_state = {false, false};

    if (mods_active != last_state.mods_active) {
        set_label_color(widget->label_mods, mods_active);
        last_state.mods_active = mods_active;
    }

    if (caps_word_active != last_state.caps_word_active) {
        set_label_color(widget->label_caps, caps_word_active);
        last_state.caps_word_active = caps_word_active;
    }
}

// ---------------------------------------------------------
// ⏱️ 주기적 업데이트 타이머
// ---------------------------------------------------------
static void mod_status_timer_handler(struct k_timer *timer) {
    struct zmk_widget_mod_status *widget = k_timer_user_data_get(timer);
    update_mod_status(widget);
}

K_TIMER_DEFINE(mod_status_timer, mod_status_timer_handler, NULL);

// ---------------------------------------------------------
// 🏗️ 초기화 함수
// ---------------------------------------------------------
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    // 모디파이어 라벨
    widget->label_mods = lv_label_create(parent);
    lv_label_set_text(widget->label_mods, LV_SYMBOL_KEYBOARD);
    lv_obj_set_style_text_font(widget->label_mods, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(widget->label_mods, LV_ALIGN_LEFT_MID, 0, 0);
    set_label_color(widget->label_mods, false);

    // Caps Word 라벨 (✅ 추가됨)
    widget->label_caps = lv_label_create(parent);
    lv_label_set_text(widget->label_caps, LV_SYMBOL_EDIT);
    lv_obj_set_style_text_font(widget->label_caps, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(widget->label_caps, LV_ALIGN_LEFT_MID, 30, 0);
    set_label_color(widget->label_caps, false);

    // 타이머에 widget 포인터 등록
    k_timer_user_data_set(&mod_status_timer, widget);

    // 500ms 간격으로 상태 확인
    k_timer_start(&mod_status_timer, K_MSEC(500), K_MSEC(500));

    return 0;
}

// ---------------------------------------------------------
// 🧱 객체 반환 함수
// ---------------------------------------------------------
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->label_mods;
}
