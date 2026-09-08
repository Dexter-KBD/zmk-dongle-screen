/* Temporary render diagnostics with one-time direct color checkpoints. */
#include <stddef.h>
#include <stdint.h>
#include <lvgl.h>
#include <lvgl_display.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(yads_render_diag, LOG_LEVEL_INF);

static atomic_t load_begin, load_done;
static atomic_t tick_begin, tick_done;
static atomic_t flush_begin, flush_done;
static atomic_t alloc_failures, failed_alloc_size;

extern void yads_spi_diagnostic_report(void);
extern void yads_diagnostic_mark(uint16_t color);
extern void __real_lv_screen_load(lv_obj_t *screen);
extern uint32_t __real_lv_timer_handler(void);
extern void __real_lvgl_flush_display(struct lvgl_display_flush *request);
extern void *__real_lvgl_malloc(size_t size);
extern void *__real_lvgl_realloc(void *ptr, size_t size);

void __wrap_lv_screen_load(lv_obj_t *screen)
{
    atomic_val_t n = atomic_inc(&load_begin);
    LOG_INF("YADS screen load begin");
    if (n == 0) {
        yads_diagnostic_mark(0xfd20); /* orange: entering screen load */
    }
    __real_lv_screen_load(screen);
    atomic_inc(&load_done);
    if (n == 0) {
        yads_diagnostic_mark(0xf81f); /* magenta: screen load returned */
    }
    LOG_INF("YADS screen load returned");
}

uint32_t __wrap_lv_timer_handler(void)
{
    atomic_val_t n = atomic_inc(&tick_begin);
    if (n < 2) {
        LOG_INF("YADS timer handler begin %d", (int)n + 1);
    }
    if (n == 0) {
        yads_diagnostic_mark(0xf800); /* red: entering first render handler */
    }
    uint32_t next = __real_lv_timer_handler();
    atomic_inc(&tick_done);
    if (n < 2) {
        LOG_INF("YADS timer handler returned %d next=%u", (int)n + 1, next);
    }
    return next;
}

void __wrap_lvgl_flush_display(struct lvgl_display_flush *request)
{
    atomic_val_t n = atomic_inc(&flush_begin);
    if (n < 4) {
        LOG_INF("YADS flush begin %d xy=%u,%u size=%u,%u bytes=%u",
                (int)n + 1, (unsigned int)request->x, (unsigned int)request->y,
                (unsigned int)request->desc.width, (unsigned int)request->desc.height,
                (unsigned int)request->desc.buf_size);
    }
    if (n == 0) {
        /* Before the real flush acquires SPI; never called from the SPI driver. */
        yads_diagnostic_mark(0x07ff); /* cyan: entering first display transfer */
    }
    __real_lvgl_flush_display(request);
    atomic_inc(&flush_done);
    if (n < 4) {
        LOG_INF("YADS flush returned %d (not proof of SPI success)", (int)n + 1);
    }
}

static void record_alloc_failure(size_t size)
{
    atomic_set(&failed_alloc_size, (atomic_val_t)size);
    atomic_val_t n = atomic_inc(&alloc_failures);
    if (n < 8) {
        LOG_ERR("YADS LVGL allocation failed: bytes=%u", (unsigned int)size);
    }
}

void *__wrap_lvgl_malloc(size_t size)
{
    void *result = __real_lvgl_malloc(size);
    if (result == NULL && size != 0) {
        record_alloc_failure(size);
    }
    return result;
}

void *__wrap_lvgl_realloc(void *ptr, size_t size)
{
    void *result = __real_lvgl_realloc(ptr, size);
    if (result == NULL && size != 0) {
        record_alloc_failure(size);
    }
    return result;
}

static void diagnostic_status(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(diagnostic_status_work, diagnostic_status);

static void diagnostic_status(struct k_work *work)
{
    ARG_UNUSED(work);
    LOG_INF("YADS state load=%d/%d tick=%d/%d flush=%d/%d (entered/returned)",
            (int)atomic_get(&load_begin), (int)atomic_get(&load_done),
            (int)atomic_get(&tick_begin), (int)atomic_get(&tick_done),
            (int)atomic_get(&flush_begin), (int)atomic_get(&flush_done));
    LOG_INF("YADS memory pool=%u alloc_failures=%d last_failed_bytes=%d",
            (unsigned int)CONFIG_LV_Z_MEM_POOL_SIZE,
            (int)atomic_get(&alloc_failures), (int)atomic_get(&failed_alloc_size));
    yads_spi_diagnostic_report();
    k_work_reschedule(&diagnostic_status_work, K_SECONDS(5));
}

static int render_diagnostic_init(void)
{
    k_work_schedule(&diagnostic_status_work, K_SECONDS(2));
    return 0;
}

SYS_INIT(render_diagnostic_init, APPLICATION, 99);
