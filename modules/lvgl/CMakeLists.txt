find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zmk_dongle_screen)

# 기존 Zephyr 라이브러리 설정 유지
zephyr_library()

# LVGL 소스 포함 (그림자 기능 사용 가능)
set(LVGL_SRC "${ZEPHYR_BASE}/modules/lvgl/lvgl.c")

# HEADER_FILE_ONLY 제거 → 실제 컴파일 포함
zephyr_library_sources(${LVGL_SRC})

# 기존 프로젝트 소스 추가
zephyr_library_sources(
    src/widgets/battery_status.c
    # 필요한 다른 소스들 여기에 추가
)
