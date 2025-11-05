# 기본 include 경로 추가
zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

# 라이브러리 생성
zephyr_library()

# dongle_screen 쉴드 전용 설정
if(CONFIG_SHIELD_dongle_screen)

    # LVGL과 Display 드라이버 서브모듈 포함
    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)
    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/modules/lvgl)

    # Caps Word 모듈 소스 항상 포함
    zephyr_library_sources(
        src/events/caps_word_state_changed.c
        ${APPLICATION_SOURCE_DIR}/src/behaviors/behavior_caps_word.c
    )

    # 기존 split/bluetooth 상태 이벤트 포함
    zephyr_library_sources(src/events/split_central_status_changed.c)
    zephyr_library_sources(src/split/bluetooth/central_status_changed_observer.c)

endif()
