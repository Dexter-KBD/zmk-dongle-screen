#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/split/central.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/event_manager.h>

#define NUM_BATTERIES (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + 1)

// 최대 캔버스 크기
#define CANVAS_WIDTH 96
#define CANVAS_HEIGHT 32

struct battery_object {
    lv_obj_t *canvas;
    lv_obj_t *label;
};

static lv_color_t battery_buffer[NUM_BATTERIES][CANVAS_WIDTH * CANVAS_HEIGHT];
static struct battery_object battery_objects[NUM_BATTERIES];
static int8_t last_battery_levels[NUM_BATTERIES];

// 배터리 색상
static lv_color_t battery_color(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    if (level <= 15) return lv_color_hex(0xFA0D0B);
    if (level <= 30) return lv_color_hex(0xF98300);
    if (level <= 40) return lv_color_hex(0xFFFF00);
    return lv_color_hex(0x08FB10);
}

// 어두운 배터리 바 색
static lv_color_t battery_color_dark(uint8_t level) {
    if (level < 1) return lv_color_hex(0x5F5CE7);
    if (level <= 15) return lv_color_hex(0xB20908);
    if (level <= 30) return lv_color_hex(0xC76A00);
    if (level <= 40) return lv_color_hex(0xB5B500);
    return lv_color_hex(0x04910A);
}

// 배터리 표시 그리기
static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    int w = CANVAS_WIDTH;
    int h = CANVAS_HEIGHT;

    int radius_main = w * 0.05; // 메인 어두운 바 radius 비율
    int border_width = 4;        // 테두리 두께

    lv_draw_rect_dsc_t outer_border;
    lv_draw_rect_dsc_init(&outer_border);
    outer_border.bg_color = lv_color_white();
    outer_border.radius = radius_main;
    outer_border.border_width = border_width;

    lv_draw_rect_dsc_t inner_border;
    lv_draw_rect_dsc_init(&inner_border);
    inner_border.bg_color = lv_color_black();
    inner_border.radius = radius_main;
    inner_border.border_width = border_width;

    lv_draw_rect_dsc_t dark_bar;
    lv_draw_rect_dsc_init(&dark_bar);
    dark_bar.bg_color = lv_color_hex(0x202020); // 메인 어두운 바
    dark_bar.radius = radius_main;

    lv_draw_rect_dsc_t bright_bar;
    lv_draw_rect_dsc_init(&bright_bar);
    bright_bar.bg_color = battery_color(level);
    bright_bar.radius = radius_main * 0.9; // 약간 작게 겹치게

    int padding = border_width; // 테두리 공간

    // 흰색 테두리
    lv_canvas_draw_rect(canvas, 0, 0, w, h, &outer_border);
    // 검정 테두리
    lv_canvas_draw_rect(canvas, padding, padding, w - padding*2, h - padding*2, &inner_border);
    // 어두운 바
    lv_canvas_draw_rect(canvas, padding*2, padding*2, w - padding*4, h - padding*4, &dark_bar);
    // 밝은 바 (잔량)
    int bright_w = (w - padding*4) * level / 100;
    lv_canvas_draw_rect(canvas, padding*2, padding*2, bright_w, h - padding*4, &bright_bar);
}

// 위젯 초기화
int zmk_widget_dongle_battery_status_init(lv_obj_t *parent) {
    for (int i = 0; i < NUM_BATTERIES; i++) {
        lv_obj_t *canvas = lv_canvas_create(parent);
        lv_canvas_set_buffer(canvas, battery_buffer[i], CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, i * (CANVAS_WIDTH + 10), 0);

        lv_obj_t *label = lv_label_create(parent);
        lv_label_set_text(label, "");
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, i * (CANVAS_WIDTH + 10), 0);

        battery_objects[i] = (struct battery_object){ .canvas = canvas, .label = label };
        last_battery_levels[i] = -1;
    }
    return 0;
}

// 업데이트
void battery_status_update(uint8_t source, uint8_t level) {
    if (source >= NUM_BATTERIES) return;
    draw_battery(battery_objects[source].canvas, level);
    lv_label_set_text_fmt(battery_objects[source].label, "%d", level); // % 제거
    lv_obj_set_style_text_color(battery_objects[source].label, lv_color_black(), 0);
}
