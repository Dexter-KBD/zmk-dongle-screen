# Include 디렉토리 설정
zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)

# 라이브러리 정의
zephyr_library()

# SHIELD가 활성화된 경우
if(CONFIG_SHIELD_PROSPECTOR_ADAPTER)

    # 하위 디렉토리 추가
    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)
    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/modules/lvgl)

    # Caps Word 기능이 활성화된 경우
    if(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
        # 실제 파일 경로에 맞게 수정
        zephyr_library_sources(
            boards/shields/dongle_screen/src/behaviors/behavior_caps_word.c
        )

        # 헤더 전용 소스 파일 등록
        set_source_files_properties(
            ${APPLICATION_SOURCE_DIR}/src/behaviors/behavior_caps_word.c
            TARGET_DIRECTORY app
            PROPERTIES HEADER_FILE_ONLY ON
        )
        target_sources(app PRIVATE src/behaviors/behavior_caps_word.c)
    endif()

    # 기타 이벤트 소스 등록
    zephyr_library_sources(
        boards/shields/dongle_screen/src/events/split_central_status_changed.c
    )
    zephyr_library_sources(
        boards/shields/dongle_screen/src/split/bluetooth/central_status_changed_observer.c
    )

endif()
