if(CONFIG_SHIELD_DONGLE_SCREEN)
  zephyr_library()

  # 더미 파일
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)

  # 포함 디렉토리
  zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
  zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_CMAKE_DIR}/include)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/zmk/app/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_CMAKE_DIR}/include) # <- fonts.h 포함
  zephyr_library_include_directories(${ZEPHYR_CURRENT_CMAKE_DIR}/include/fonts) # <- 여기 추가!

  # 소스 파일
  zephyr_library_sources(src/brightness.c)
  zephyr_library_sources(src/custom_status_screen.c)
  zephyr_library_sources(src/screen_rotate_init.c)
  zephyr_library_sources(src/widgets/output_status.c)
  zephyr_library_sources(src/widgets/battery_status.c)
  zephyr_library_sources(src/widgets/layer_status.c)
  zephyr_library_sources(src/widgets/wpm_status.c)
  zephyr_library_sources(src/widgets/mod_status.c)
  zephyr_library_sources(src/widgets/signal_status.c)

  # 폰트 소스
  file(GLOB font_sources src/fonts/*.c)
  zephyr_library_sources(${font_sources})
endif()
