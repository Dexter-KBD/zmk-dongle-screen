# ==============================
# zmk-dongle-screen CMakeLists.txt
# ==============================

cmake_minimum_required(VERSION 3.13.1)

# Zephyr 패키지 찾기
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zmk_dongle_screen)

# 기존 Zephyr 라이브러리 설정 유지
zephyr_library()

# 🔹 LVGL 소스 포함 (그림자 기능 사용 가능)
set(LVGL_SRC "${ZEPHYR_BASE}/modules/lvgl/lvgl.c")
zephyr_library_sources(${LVGL_SRC})

# 🔹 프로젝트 소스 추가
# 실제 파일 경로 기준으로 수정: battery_status.c 위치
zephyr_library_sources(
    boards/shields/dongle_screen/src/widgets/battery_status.c
    # 필요하면 다른 소스도 추가 가능
)

# ==============================
# 설명:
# - LVGL_SRC: LVGL 모듈의 lvgl.c를 포함하여 그림자 기능 등 모든 LVGL 기능 사용 가능
# - zephyr_library_sources: 실제 소스 컴파일 포함
# - battery_status.c: repo 구조에 맞게 경로 지정
# ==============================
