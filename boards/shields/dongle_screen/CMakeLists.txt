if(CONFIG_SHIELD_DONGLE_SCREEN)
  zephyr_library()
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)

  # 기존 include 경로
  zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
  zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_CMAKE_DIR}/include)
  zephyr_library_include_directories(include)

  # 추가: behavior_caps_word.h 포함 경로
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include/zmk) # zmk/events 경로 포함

  # 기존 소스 파일
  zephyr_library_sources(src/brightness.c)
  zephyr_library_sources(src/custom_status_screen.c)
  zephyr_library_sources(src/screen_rotate_init.c)
  zephyr_library_sources(src/widgets/output_status.c)
  zephyr_library_sources(src/widgets/battery_status.c)
  zephyr_library_sources(src/widgets/layer_status.c)
  zephyr_library_sources(src/widgets/wpm_status.c)
  zephyr_library_sources(src/widgets/mod_status.c)

  # 추가: behavior_caps_word.c
  zephyr_library_sources(src/behaviors/behavior_caps_word.c)

  # 기존 font 파일
  file(GLOB font_sources src/fonts/*.c)
  zephyr_library_sources(${font_sources})
endif()
