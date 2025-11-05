# ===============================
# dongle_screen용 CMakeLists.txt
# ===============================

# include 경로
zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)

# 라이브러리 생성
zephyr_library()

# ===============================
# 위젯 소스 직접 포함
# ===============================
zephyr_library_sources(
    src/widgets/mod_status.c
    src/widgets/layer_status.c
    src/widgets/output_status.c
    src/widgets/battery_status.c
    src/widgets/wpm_status.c
    src/brightness.c
)

# ===============================
# Caps Word 이벤트 관련
# ===============================
if(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
    zephyr_library_sources(src/events/caps_word_state_changed.c)

    # behavior_caps_word.c는 header-only 처리
    set_source_files_properties(
        ${APPLICATION_SOURCE_DIR}/src/behaviors/behavior_caps_word.c
        PROPERTIES HEADER_FILE_ONLY ON
    )

    target_sources(app PRIVATE src/behaviors/behavior_caps_word.c)
endif()

# ===============================
# Split / Bluetooth 관련
# ===============================
zephyr_library_sources(
    src/events/split_central_status_changed.c
    src/split/bluetooth/central_status_changed_observer.c
)
