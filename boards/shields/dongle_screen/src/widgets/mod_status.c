#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/caps_word.h>          // ✅ Caps Word 상태 확인용
#include <zmk/keys.h>
#include <zmk/keymap.h>
#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <lvgl.h>

// ========================================================
// 구조체 및 위젯 핸들
// ========================================================
static lv_obj_t *mod_label;
static lv_obj_t *caps_word_label;

// ========================================================
// 헬퍼 함수: modifier 상태 텍스트 생성
// ========================================================
static void update_mod_text(void) {
    zmk_mod_flags_t mods = zmk_mod_flags();

    // 초기화
    static char mod_text[32];
    mod_text[0] = '\0';

    // 각 modifier 상태를 확인
    if (mods & (ZMK_MOD_LSFT | ZMK_MOD_RSFT)) {
        strcat(mod_text, "⇧ ");
    }
    if (mods & (ZMK_MOD_LCTL | ZMK_MOD_RCTL)) {
        strcat(mod_text, "⌃ ");
    }
    if (mods & (ZMK_MOD_LALT | ZMK_MOD_RALT)) {
        strcat(mod_text, "⎇ ");
    }
    if (mods & (ZMK_MOD_LGUI | ZMK_MOD_RGUI)) {
        strcat(mod_text, "⌘ ");
    }

    // 표시
    if (strlen(mod_text) == 0) {
        strcpy(mod_text, "-");
    }

    lv_label_set_text(mod_label, mod_text);
}

// ========================================================
// Caps Word 상태 업데이트
// ========================================================
static void update_caps_word_indicator(void) {
    bool caps_active = zmk_caps_word_get_state();  // ✅ Caps Word 상태 직접 읽기

    if (caps_active) {
        lv_obj_set_style_text_color(caps_word_label, lv_color_hex(0x00FFE5), 0); // 밝은 시안색
    } else {
        lv_obj_set_style_text_color(caps_word_label, lv_color_hex(0x303030), 0); // 어두운 회색
    }
}

// ========================================================
// 전체 상태 업데이트
// ========================================================
static void update_mod_status(void) {
    update_mod_text();
    update_caps_word_indicator();
}

// ========================================================
// LVGL 위젯 초기화
// ========================================================
static void init_mod_status(lv_obj_t *parent) {
    // modifier 표시용 라벨
    mod_label = lv_label_create(parent);
    lv_obj_set_style_text_font(mod_label, &lv_font_montserrat_20, 0);
    lv_obj_align(mod_label, LV_ALIGN_TOP_LEFT, 4, 0);

    // caps word 표시용 라벨
    caps_word_label = lv_label_create(parent);
    lv_label_set_text(caps_word_label, LV_SYMBOL_KEYBOARD " CW");
    lv_obj_set_style_text_font(caps_word_label, &lv_font_montserrat_20, 0);
    lv_obj_align(caps_word_label, LV_ALIGN_TOP_RIGHT, -8, 0);

    update_mod_status();
}

// ========================================================
// 이벤트 리스너 (modifier 바뀔 때 호출됨)
// ========================================================
static int on_modifiers_state_changed(const zmk_event_t *eh) {
    update_mod_status();
    return 0;
}

ZMK_LISTENER(mod_status, on_modifiers_state_changed);
ZMK_SUBSCRIPTION(mod_status, zmk_modifiers_state_changed);

// ========================================================
// 초기화 등록
// ========================================================
static int mod_status_init(const struct device *dev) {
    ARG_UNUSED(dev);
    lv_obj_t *screen = zmk_display_status_screen();
    init_mod_status(screen);
    return 0;
}

SYS_INIT(mod_status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
