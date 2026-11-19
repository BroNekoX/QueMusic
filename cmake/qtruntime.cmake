# 统一的 Qt 运行时部署：链接后把 Qt 运行库放到可执行文件旁边
function(qt_deploy_runtime target)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

        if(WINDEPLOYQT_EXECUTABLE)
            # --no-opengl-sw 不部署软件 OpenGL；--no-translations 工程无 QTranslator
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                        --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
                        --no-opengl-sw
                        --no-translations
                        "$<TARGET_FILE:${target}>"
                COMMENT "[qt_deploy_runtime] Deploying Qt runtime (windeployqt)")

            # 裁剪 windeployqt 全量部署里用不到的组件（清单见 cmake/slim_deployment.cmake）
            if(QUEMUSIC_SLIM_RUNTIME)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND "${CMAKE_COMMAND}"
                            "-DQM_DEPLOY_DIR=$<TARGET_FILE_DIR:${target}>"
                            -P "${CMAKE_SOURCE_DIR}/cmake/slim_deployment.cmake"
                    COMMENT "[qt_deploy_runtime] 精简未使用的 Qt 运行时组件")
            endif()
        else()
            message(WARNING "[qt_deploy_runtime] windeployqt not found, skipped")
        endif()

    elseif(APPLE)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

        if(MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                        "$<TARGET_BUNDLE_DIR:${target}>"
                COMMENT "[qt_deploy_runtime] Deploying Qt runtime (macdeployqt)")
        else()
            message(WARNING "[qt_deploy_runtime] macdeployqt not found, skipped")
        endif()

    elseif(UNIX)
        find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)
        if(LINUXDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${LINUXDEPLOYQT_EXECUTABLE}"
                        "$<TARGET_FILE:${target}>"
                        -qmldir="${CMAKE_CURRENT_SOURCE_DIR}"
                        -appimage
                COMMENT "[qt_deploy_runtime] Deploying Qt runtime (linuxdeployqt)")
        else()
            message(STATUS "[qt_deploy_runtime] linuxdeployqt not found, assuming system Qt runtime")
        endif()
    endif()
endfunction()
