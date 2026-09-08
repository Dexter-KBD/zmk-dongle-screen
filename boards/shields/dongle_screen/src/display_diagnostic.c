/* Temporary direct-display diagnostic: runs after panel rotation, before LVGL. */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/led.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define DIAG_DISPLAY_NODE DT_CHOSEN(zephyr_display)
#define DIAG_MAX_WIDTH MAX(DT_PROP(DIAG_DISPLAY_NODE, width), DT_PROP(DIAG_DISPLAY_NODE, height))

/* Static storage avoids adding a scanline to the small initialization stack. */
static uint8_t diagnostic_line[DIAG_MAX_WIDTH * 2] __aligned(4);

static int direct_display_diagnostic(void)
{
    const struct device *panel = DEVICE_DT_GET(DIAG_DISPLAY_NODE);
    const struct device *backlight = DEVICE_DT_GET_ONE(pwm_leds);
    struct display_capabilities cap;
    static const uint16_t colors[] = {0xf800, 0x07e0, 0x001f};
    int ret;

    if (!device_is_ready(panel) || !device_is_ready(backlight)) {
        return -ENODEV;
    }

    display_get_capabilities(panel, &cap);
    if (cap.current_pixel_format != PIXEL_FORMAT_RGB_565) {
        return -ENOTSUP;
    }

    uint16_t width = cap.x_resolution;
    uint16_t height = cap.y_resolution;
    if (cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_90 ||
        cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_270) {
        width = cap.y_resolution;
        height = cap.x_resolution;
    }
    if (width == 0 || height == 0 || width > DIAG_MAX_WIDTH) {
        return -EINVAL;
    }

    ret = led_set_brightness(backlight, DT_NODE_CHILD_IDX(DT_NODELABEL(disp_bl)),
                            CONFIG_DONGLE_SCREEN_DEFAULT_BRIGHTNESS);
    if (ret < 0) {
        return ret;
    }

    const struct display_buffer_descriptor desc = {
        .buf_size = width * 2U,
        .width = width,
        .height = 1,
        .pitch = width,
    };

    for (unsigned int cycle = 0; cycle < 2; ++cycle) {
        for (unsigned int color = 0; color < ARRAY_SIZE(colors); ++color) {
            /* The direct SPI driver expects RGB565 bytes in panel wire order. */
            for (uint16_t x = 0; x < width; ++x) {
                diagnostic_line[x * 2U] = colors[color] >> 8;
                diagnostic_line[x * 2U + 1U] = colors[color] & 0xff;
            }
            for (uint16_t y = 0; y < height; ++y) {
                ret = display_write(panel, 0, y, &desc, diagnostic_line);
                if (ret < 0) {
                    return ret;
                }
            }
            ret = display_blanking_off(panel);
            if (ret < 0) {
                return ret;
            }
            k_sleep(K_SECONDS(2));
        }
    }

    /* Leave blue visible until the normal UI draws its first frame. */
    return 0;
}

/* Existing panel rotation is APPLICATION/60; LVGL initialization is /90. */
SYS_INIT(direct_display_diagnostic, APPLICATION, 80);

/* Called only during initialization, before regular LVGL refreshes start. */
void yads_diagnostic_mark(uint16_t color)
{
    const struct device *panel = DEVICE_DT_GET(DIAG_DISPLAY_NODE);
    struct display_capabilities cap;
    if (!device_is_ready(panel)) {
        return;
    }
    display_get_capabilities(panel, &cap);
    if (cap.current_pixel_format != PIXEL_FORMAT_RGB_565) {
        return;
    }
    uint16_t width = cap.x_resolution;
    uint16_t height = cap.y_resolution;
    if (cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_90 ||
        cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_270) {
        width = cap.y_resolution;
        height = cap.x_resolution;
    }
    if (width == 0 || height == 0 || width > DIAG_MAX_WIDTH) {
        return;
    }
    for (uint16_t x = 0; x < width; ++x) {
        diagnostic_line[x * 2U] = color >> 8;
        diagnostic_line[x * 2U + 1U] = color & 0xff;
    }
    const struct display_buffer_descriptor desc = {
        .buf_size = width * 2U,
        .width = width,
        .height = 1,
        .pitch = width,
    };
    for (uint16_t y = 0; y < height; ++y) {
        if (display_write(panel, 0, y, &desc, diagnostic_line) < 0) {
            return;
        }
    }
    display_blanking_off(panel);
    k_sleep(K_MSEC(100));
}
