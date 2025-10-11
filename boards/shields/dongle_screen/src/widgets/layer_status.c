/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state
{
    uint8_t index;
    const char *label;
};

static void set_layer_symbol(lv_obj_t *label, struct layer_status_state state)
{
    if (state.label == NULL)
    {
        char text[7] = {};

        sprintf(text, "%i", state.index);

        lv_label_set_text(label, text);
    }
    else
    {
        char text[13] = {};

        snprintf(text, sizeof(text), "%s", state.label);

        lv_label_set_text(label, text);
    }
         // 레이어별 색상 지정
 lv_color_t color;
    
    switch (state.index)
    {
        case 0:
            color = lv_color_hex(0xA8E6CF); // 민트 (기본 레이어)
            break;
        case 1:
            color = lv_color_hex(0xDCEDC1); // 연한 그린
            break;
        case 2:
            color = lv_color_hex(0xFFD3B6); // 살구
            break;
        case 3:
            color = lv_color_hex(0xFFAAA5); // 코랄 핑크
            break;
        case 4:
             color = lv_color_hex(0xFFE082); // 크림 옐로우
            break;
        case 5:
            color = lv_color_hex(0xDCEDC1); // 연한 그린
            break;
        case 6:
            color = lv_color_hex(0xB3E5FC); // 하늘색
            break;
        case 7:
            color = lv_color_hex(0xF8BBD0); // 로즈 핑크
            break;
        case 8:
            color = lv_color_hex(0xFFE082); // 크림 옐로우
            break;
        default:
            color = lv_color_hex(0xFFFFFF); // 예외: 흰색
            break;
    }

    // 텍스트 색상 적용
    lv_obj_set_style_text_color(label, color, 0);
}

static void layer_status_update_cb(struct layer_status_state state)
{
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_symbol(widget->obj, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh)
{
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index,
        .label = zmk_keymap_layer_name(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_label_create(parent);

    lv_obj_set_style_text_font(widget->obj, &lv_font_montserrat_40, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget)
{
    return widget->obj;
}
