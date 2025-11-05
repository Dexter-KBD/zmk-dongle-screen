# 현재 보드 CMakeLists.txt 시작 부분
zephyr_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include   # app/include 경로
    ${CMAKE_CURRENT_SOURCE_DIR}/include              # dongle_screen/include 경로
)

# 라이브러리 선언
zephyr_library()

# caps word behavior 사용 시
if(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
    zephyr_library_sources(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/behaviors/behavior_caps_word.c
    )
endif()

# 기타 이벤트 소스 추가
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/widgets/mod_status.c
)
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/widgets/battery_status.c
)
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/widgets/layer_status.c
)
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/widgets/output_status.c
)
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/brightness.c
)
zephyr_library_sources(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../boards/shields/dongle_screen/src/widgets/wpm_status.c
)
