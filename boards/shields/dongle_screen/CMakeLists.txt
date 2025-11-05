if(CONFIG_SHIELD_DONGLE_SCREEN)
  zephyr_library()
  
  # 기본 소스 파일 (원래 있던 것 그대로)
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)
  zephyr_library_sources(src/brightness.c)
  zephyr_library_sources(src/custom_status_screen.c)
  zephyr_library_sources(src/screen_rotate_init.c)
  zephyr_library_sources(src/widgets/output_status.c)
  zephyr_library_sources(src/widgets/battery_status.c)
  zephyr_library_sources(src/widgets/layer_status.c)
  zephyr_library_sources(src/widgets/wpm_status.c)
  zephyr_library_sources(src/widgets/mod_status.c)

  # 새로 추가: behavior_caps_word.c
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/behaviors/behavior_caps_word.c)

  # 글꼴 소스
  file(GLOB font_sources src/fonts/*.c)
  zephyr_library_sources(${font_sources})

  # include 디렉토리
  zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
  zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)          # 루트 include
  zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR}/include)    # 보드/실드 include
  zephyr_library_include_directories(include)                               # 상대 include
endif()
