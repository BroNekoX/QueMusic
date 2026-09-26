# 统一的 Qt 运行时部署：链接后把 Qt 运行库放到可执行文件旁边
function(qt_deploy_runtime target)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

        if(WINDEPLOYQT_EXECUTABLE)
            # --no-opengl-sw：不部署软件 OpenGL 兜底（约 20 MB，走 D3D11 时永不加载）
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                        --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
                        --no-opengl-sw
                        "$<TARGET_FILE:${target}>"
                COMMENT "[qt_deploy_runtime] Deploying Qt runtime (windeployqt)")

            # 数据库只用 QSQLITE（见 cpp/DbService.cpp）。windeployqt 会把整个 sqldrivers
            # 目录拷进来，连带 MySQL 客户端与其 OpenSSL 依赖（共约 16 MB），这些插件
            # 从不被加载，属于纯安装体积。保留 qsqlite.dll，其余删掉。
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E rm -f
                        # 旧部署残留：windeployqt 只“不新增”，不会删除已经存在的文件
                        "$<TARGET_FILE_DIR:${target}>/opengl32sw.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/qsqlmysql.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/qsqloci.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/qsqlodbc.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/qsqlpsql.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/qsqlmimer.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/qsqlibase.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/libmysql.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/libssl-3-x64.dll"
                        "$<TARGET_FILE_DIR:${target}>/sqldrivers/libcrypto-3-x64.dll"
                COMMENT "[qt_deploy_runtime] 精简未使用的 SQL 驱动 / MySQL / OpenSSL")
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
