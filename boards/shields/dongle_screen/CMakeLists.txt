if(CONFIG_SHIELD_DONGLE_SCREEN)
  # --------------------
  # 라이브러리 선언
  # --------------------
  zephyr_library()

  # --------------------
  # 더미 파일 (필요 시)
  # --------------------
  zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)

  # --------------------
  # 포함 디렉토리
  # --------------------
  zephyr_library_include_directories(${ZEPHYR_LVGL_MODULE_DIR})                 # LVGL 모듈
  zephyr_library_include_directories(${ZEPHYR_BASE}/lib/gui/lvgl/)               # LVGL 기본
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)                     # Zephyr 드라이버
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)                # 프로젝트 루트 include
  zephyr_library_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)       # 현재 모듈 include
  zephyr_library_include_directories(${ZEPHYR_CURRENT_CMAKE_DIR}/include)        # 현재 CMake 디렉토리 include
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/zmk/app/include)        # ZMK 이벤트 헤더 포함

  # --------------------
  # 소스 파일
  # --------------------
  zephyr_library_sources(src/brightness.c)
  zephyr_library_sources(src/custom_status_screen.c)
  zephyr_library_sources(src/screen_rotate_init.c)

  zephyr_library_sources(src/widgets/output_status.c)
  zephyr_library_sources(src/widgets/battery_status.c)
  zephyr_library_sources(src/widgets/layer_status.c)
  zephyr_library_sources(src/widgets/wpm_status.c)
  zephyr_library_sources(src/widgets/mod_status.c)
  zephyr_library_sources(src/widgets/signal_status.c)

  # --------------------
  # 폰트 소스
  # --------------------
  file(GLOB font_sources src/fonts/*.c)
  zephyr_library_sources(${font_sources})
endif()
