if(CONFIG_SHIELD_DONGLE_SCREEN)
  # 라이브러리 생성
  zephyr_library()

  # 기본 빈 파일 포함 (Zephyr 필수)
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)

  # include 디렉토리 추가
  zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})
  zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)
  zephyr_library_include_directories(${ZEPHYR_CURRENT_CMAKE_DIR}/include)
  zephyr_library_include_directories(include)

  # 소스 파일 추가 (절대 경로 사용)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/brightness.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/custom_status_screen.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/screen_rotate_init.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/widgets/output_status.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/widgets/battery_status.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/widgets/layer_status.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/widgets/wpm_status.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/widgets/mod_status.c)
  zephyr_library_sources(${CMAKE_CURRENT_LIST_DIR}/src/behaviors/behavior_caps_word.c)

  # 폰트 소스 파일 자동 추가
  file(GLOB font_sources ${CMAKE_CURRENT_LIST_DIR}/src/fonts/*.c)
  zephyr_library_sources(${font_sources})
endif()
