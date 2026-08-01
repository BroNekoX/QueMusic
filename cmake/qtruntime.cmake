# 统一 Qt 部署函数
function(qt_deploy_runtime target)
    if(WIN32)
        # Windows 平台用 windeployqt
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                        --verbose 1
                        --no-compiler-runtime
                        --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
                        "$<TARGET_FILE:${target}>"
                COMMENT "[qt_deploy_runtime] Deploying Qt libraries on Windows...")
        else()
            message(WARNING "[qt_deploy_runtime] windeployqt not found, skipped.")
        endif()

    elseif(APPLE)
        # macOS 平台用 macdeployqt
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")

        if(MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                        "$<TARGET_BUNDLE_DIR:${target}>"
                COMMENT "[qt_deploy_runtime] Deploying Qt libraries on macOS...")
        else()
            message(WARNING "[qt_deploy_runtime] macdeployqt not found, skipped.")
        endif()

    elseif(UNIX)
        # Linux 平台用 linuxdeployqt
        find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)
        if(LINUXDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${LINUXDEPLOYQT_EXECUTABLE}"
                        "$<TARGET_FILE:${target}>"
                        -qmldir="${CMAKE_CURRENT_SOURCE_DIR}"
                        -appimage
                COMMENT "[qt_deploy_runtime] Deploying Qt libraries on Linux...")
        else()
            message(STATUS "[qt_deploy_runtime] linuxdeployqt not found, assuming system Qt runtime.")
        endif()
    endif()
endfunction()
