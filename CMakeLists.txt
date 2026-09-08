zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library()

if(CONFIG_SHIELD_DONGLE_SCREEN)

        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)

        if(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
                zephyr_library_sources(src/events/caps_word_state_changed.c)
                # APPLICATION_SOURCE_DIR is the ZMK app root in a ZMK build;
                # mark the stock source out before adding the event-emitting copy.
                set_source_files_properties(
                        ${APPLICATION_SOURCE_DIR}/src/behaviors/behavior_caps_word.c
                        TARGET_DIRECTORY app
                        PROPERTIES HEADER_FILE_ONLY ON)
                target_sources(app PRIVATE src/behaviors/behavior_caps_word.c)
        endif()

endif()
