# Dongle shield library
zephyr_library()

# Include 디렉토리 설정
zephyr_include_directories(
    ${ZEPHYR_BASE}/include                        # Zephyr 기본 include
    ${CMAKE_CURRENT_SOURCE_DIR}/include           # dongle_screen/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include  # zmk/app/include (zmk/events 등)
)

# Behaviors
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/behaviors/behavior_caps_word.c
)

# Widgets
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/mod_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/battery_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/layer_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/output_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/wpm_status.c
)

# 기타 소스
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/brightness.c
)
