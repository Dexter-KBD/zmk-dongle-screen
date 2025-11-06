#include "mod_status.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// 위젯 리스트
static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// 위젯 상태 구조체
struct mod_status_state {
    bool caps_active;
};

// 라벨 색상 설정
static void mod_status_set_caps(lv_obj_t *label, bool active) {
    if (active) {
        lv_obj_set_style_text_color(label, lv_color_hex(0x00ffe5), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(label, lv_color_hex(0x202020), LV_PART_MAIN);
    }
}

// 모든 위젯 업데이트
static void mod_status_update_widgets(struct mod_status_state state) {
    struct zmk_widget_mod_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        mod_status_set_caps(widget->obj, state.caps_active);
    }
}

// 이벤트에서 상태 추출 (임시, 실제 이벤트 연결 전)
static struct mod_status_state mod_status_get_state(bool caps_active) {
    return (struct mod_status_state){
        .caps_active = caps_active,
    };
}

// 위젯 초기화
int zmk_widget_mod_status_init(struct zmk_widget_mod_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    lv_label_set_text(widget->obj, "CAPS");  // 표시용 텍스트
    lv_obj_set_style_text_color(widget->obj, lv_color_hex(0x202020), LV_PART_MAIN);

    sys_slist_append(&widgets, &widget->node);

    // 임시로 초기 상태 반영
    struct mod_status_state state = mod_status_get_state(false);
    mod_status_update_widgets(state);

    return 0;
}

// 객체 접근
lv_obj_t *zmk_widget_mod_status_obj(struct zmk_widget_mod_status *widget) {
    return widget->obj;
}
