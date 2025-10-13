#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BATTERY_WIDTH 102
#define BATTERY_HEIGHT 20

#define OUTER_WHITE_THICKNESS 4
#define OUTER_BLACK_THICKNESS 4
#define INNER_RADIUS 5

#define NUM_BATTERIES (ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT + 1) // 중앙 포함

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
    rect_dsc.border_width = 0;

    // 밝은 배터리 영역
    int inner_x = OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS;
    int inner_y = OUTER_WHITE_THICKNESS + OUTER_BLACK_THICKNESS;
    int inner_w = BATTERY_WIDTH - 2*inner_x;
    int inner_h = BATTERY_HEIGHT - 2*inner_y;
    int bright_w = inner_w * level / 100;

    rect_dsc.bg_color = battery_color(level);
    rect_dsc.radius = INNER_RADIUS;
    lv_canvas_draw_rect(canvas, inner_x, inner_y, bright_w, inner_h, &rect_dsc);

    // 어두운 배터리 영역
    rect_dsc.bg_color = battery_color_dark(level);
    lv_canvas_draw_rect(canvas, inner_x + bright_w, inner_y, inner_w - bright_w, inner_h, &rect_dsc);

    // 흰 테두리
    rect_dsc.bg_color = lv_color_white();
    rect_dsc.radius = INNER_RADIUS + OUTER_BLACK_THICKNESS;
    lv_canvas_draw_rect(canvas, 0, 0, BATTERY_WIDTH, BATTERY_HEIGHT, &rect_dsc);

    // 검정 테두리
    rect_dsc.bg_color = lv_color_black();
    rect_dsc.radius = INNER_RADIUS + OUTER_BLACK_THICKNESS + OUTER_WHITE_THICKNESS;
    lv_canvas_draw_rect(canvas, -OUTER_WHITE_THICKNESS, -OUTER_WHITE_THICKNESS,
                        BATTERY_WIDTH + OUTER_WHITE_THICKNESS*2,
                        BATTERY_HEIGHT + OUTER_WHITE_THICKNESS*2,
                        &rect_dsc);

    // 숫자 중앙 표시
    char buf[8];
    if(level < 1) snprintf(buf, sizeof(buf), "sleep");
    else snprintf(buf, sizeof(buf), "%u%%", level);

    lv_canvas_draw_text(canvas,
                        BATTERY_WIDTH/2 - 20,
                        BATTERY_HEIGHT/2 - 10,
                        40,
                        lv_font_default(),
                        buf,
                        LV_LABEL_ALIGN_CENTER);
}

// 배터리 심볼 + 레이블 설정
static void set_battery_symbol(struct battery_object *obj, uint8_t level) {
    draw_battery(obj->canvas, level);
    lv_label_set_text(obj->label, ""); // 숫자는 캔버스 안쪽에 그림
}

// 초기화
int battery_widget_init(lv_obj_t *parent) {
    for (int i = 0; i < NUM_BATTERIES; i++) {
        lv_obj_t *canvas = lv_canvas_create(parent);
        lv_canvas_set_buffer(canvas, battery_canvas_buf[i], BATTERY_WIDTH, BATTERY_HEIGHT, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(canvas, LV_ALIGN_CENTER, (i - NUM_BATTERIES/2) * (BATTERY_WIDTH + 10), 0);

        lv_obj_t *label = lv_label_create(parent);
        lv_obj_align(label, LV_ALIGN_CENTER, (i - NUM_BATTERIES/2) * (BATTERY_WIDTH + 10), 0);

        battery_objects[i].canvas = canvas;
        battery_objects[i].label = label;
    }
    return 0;
}

// 이벤트 콜백: 중앙 배터리
static void handle_central_battery_event(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if(!ev) return;
    battery_update(0, ev->state_of_charge); // 0 = 중앙
}

// 이벤트 콜백: 퍼리퍼럴 배터리
static void handle_peripheral_battery_event(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    if(!ev) return;
    battery_update(ev->source + 1, ev->state_of_charge); // +1 = 중앙 뒤
}

// 배터리 업데이트
void battery_update(uint8_t source, uint8_t level) {
    if (source >= NUM_BATTERIES) return;
    set_battery_symbol(&battery_objects[source], level);
}

// ZMK 이벤트 구독
ZMK_SUBSCRIPTION(battery_widget, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(battery_widget, zmk_peripheral_battery_state_changed);
