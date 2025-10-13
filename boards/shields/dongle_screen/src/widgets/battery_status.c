#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BATTERY_WIDTH 96
#define BATTERY_HEIGHT 32

#define OUTER_WHITE_THICKNESS 4
#define OUTER_BLACK_THICKNESS 4
#define INNER_RADIUS 8

#define NUM_BATTERIES 2

static lv_color_t battery_canvas_buf[NUM_BATTERIES][BATTERY_WIDTH * BATTERY_HEIGHT];

struct battery_object {
    lv_obj_t *canvas;
    lv_obj_t *label;
};
static struct battery_object battery_objects[NUM_BATTERIES];

// 배터리 색상 정의 (밝은색)
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xFA0D0B);
    else if (level <= 30) return lv_color_hex(0xF98300);
    else if (level <= 40) return lv_color_hex(0xFFFF00);
    else return lv_color_hex(0x08FB10);
}

// 배터리 색상 정의 (어두운색)
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    else if (level <= 15) return lv_color_hex(0xB20908);
    else if (level <= 30) return lv_color_hex(0xC76A00);
    else if (level <= 40) return lv_color_hex(0xB5B500);
    else return lv_color_hex(0x04910A);
}

// 배터리 막대 그리기
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);

    /* 흰색 외곽 */
    rect_dsc.bg_color = lv_color_white();
    rect_dsc.radius = OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS + INNER_RADIUS;
    lv_canvas_draw_rect(canvas, 0, 0, BATTERY_WIDTH, BATTERY_HEIGHT, &rect_dsc);

    /* 검정 외곽 */
    rect_dsc.bg_color = lv_color_black();
    rect_dsc.radius = OUTER_BLACK_THICKNESS + INNER_RADIUS;
    lv_canvas_draw_rect(canvas, OUTER_WHITE_THICKNESS, OUTER_WHITE_THICKNESS,
                        BATTERY_WIDTH - 2 * OUTER_WHITE_THICKNESS,
                        BATTERY_HEIGHT - 2 * OUTER_WHITE_THICKNESS, &rect_dsc);

    /* 메인 배터리 막대 (어두운색) */
    rect_dsc.bg_color = battery_color_dark(level);
    rect_dsc.radius = INNER_RADIUS;
    lv_canvas_draw_rect(canvas, OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS,
                        OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS,
                        BATTERY_WIDTH - 2 * (OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS),
                        BATTERY_HEIGHT - 2 * (OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS),
                        &rect_dsc);

    /* 밝은 잔량 바 */
    if (level > 0) {
        rect_dsc.bg_color = battery_color(level);
        lv_coord_t fill_width = (BATTERY_WIDTH - 2 * (OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS)) * level / 100;
        lv_canvas_draw_rect(canvas, OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS,
                            OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS,
                            fill_width,
                            BATTERY_HEIGHT - 2 * (OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS),
                            &rect_dsc);
    }
}

// 배터리 심볼 + 레이블 설정
static void set_battery_symbol(struct battery_object *obj, uint8_t level) {
    draw_battery(obj->canvas, level);

    char buf[6];
    if (level < 1) strcpy(buf, "sleep");
    else snprintf(buf, sizeof(buf), "%u", level);

    lv_label_set_text(obj->label, buf);
    lv_obj_set_style_text_color(obj->label, lv_color_black(), 0);
}

// 초기화
int battery_widget_init(lv_obj_t *parent) {
    for (int i = 0; i < NUM_BATTERIES; i++) {
        lv_obj_t *canvas = lv_canvas_create(parent);
        lv_canvas_set_buffer(canvas, battery_canvas_buf[i], BATTERY_WIDTH, BATTERY_HEIGHT, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(canvas, LV_ALIGN_CENTER, (i - NUM_BATTERIES/2)* (BATTERY_WIDTH + 10), 0);

        lv_obj_t *label = lv_label_create(parent);
        lv_obj_align(label, LV_ALIGN_CENTER, (i - NUM_BATTERIES/2) * (BATTERY_WIDTH + 10), 0);

        battery_objects[i].canvas = canvas;
        battery_objects[i].label = label;
    }
    return 0;
}

// 업데이트 콜백
void battery_update(uint8_t source, uint8_t level) {
    if (source >= NUM_BATTERIES) return;
    set_battery_symbol(&battery_objects[source], level);
}
