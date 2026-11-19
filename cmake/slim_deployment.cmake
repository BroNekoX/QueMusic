# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2025-2026 QueMusic Contributors
#
# 精简 windeployqt 部署产物（-DQM_DRY_RUN=ON 只打印不删除）：
#   cmake -DQM_DEPLOY_DIR=<bin> -P cmake/slim_deployment.cmake
#
# Quick Controls 只留 Basic（QtQuick.Controls.qmldir 默认导入它）；
# data/ 和 logs/ 是用户运行期数据，不在此处理。

if(NOT DEFINED QM_DEPLOY_DIR)
    message(FATAL_ERROR "slim_deployment.cmake: 需要 -DQM_DEPLOY_DIR=<部署目录>")
endif()

if(NOT IS_DIRECTORY "${QM_DEPLOY_DIR}")
    message(STATUS "[slim_deployment] 目录不存在，跳过：${QM_DEPLOY_DIR}")
    return()
endif()

if(NOT DEFINED QM_DRY_RUN)
    set(QM_DRY_RUN OFF)
endif()

function(_qm_path_size path out)
    if(IS_DIRECTORY "${path}")
        file(GLOB_RECURSE _qm_files LIST_DIRECTORIES false "${path}/*")
        set(_qm_sum 0)
        foreach(_qm_f IN LISTS _qm_files)
            file(SIZE "${_qm_f}" _qm_s)
            math(EXPR _qm_sum "${_qm_sum} + ${_qm_s}")
        endforeach()
    elseif(EXISTS "${path}")
        file(SIZE "${path}" _qm_sum)
    else()
        set(_qm_sum 0)
    endif()
    set(${out} "${_qm_sum}" PARENT_SCOPE)
endfunction()

set(_qm_remove_paths
    # Quick Controls：只留 Basic
    qml/QtQuick/Controls/FluentWinUI3
    qml/QtQuick/Controls/Windows
    qml/QtQuick/Controls/Fusion
    qml/QtQuick/Controls/Material
    qml/QtQuick/Controls/Universal
    qml/QtQuick/Controls/Imagine
    Qt6QuickControls2Fusion.dll
    Qt6QuickControls2Material.dll
    Qt6QuickControls2Universal.dll
    Qt6QuickControls2Imagine.dll
    Qt6QuickControls2FusionStyleImpl.dll
    Qt6QuickControls2MaterialStyleImpl.dll
    Qt6QuickControls2UniversalStyleImpl.dll
    Qt6QuickControls2ImagineStyleImpl.dll
    Qt6QuickControls2FluentWinUI3StyleImpl.dll
    Qt6QuickControls2WindowsStyleImpl.dll

    qml/QtQuick/NativeStyle
    translations
    qmltooling

    # Lottie / VectorImage / Quick3D
    Qt6Lottie.dll
    Qt6LottieVectorImageGenerator.dll
    Qt6Quick3DUtils.dll
    Qt6QuickVectorImageGenerator.dll
    vectorimageformats
    qml/QtQuick/VectorImage

    # 虚拟键盘 / TUIO
    Qt6VirtualKeyboard.dll
    platforminputcontexts
    generic

    qml/QML

    # 只留 jpeg/gif/svg/ico/webp
    imageformats/qicns.dll
    imageformats/qtga.dll
    imageformats/qtiff.dll
    imageformats/qwbmp.dll

    # 旧部署残留
    opengl32sw.dll
    sqldrivers/qsqlmysql.dll
    sqldrivers/qsqloci.dll
    sqldrivers/qsqlodbc.dll
    sqldrivers/qsqlpsql.dll
    sqldrivers/qsqlmimer.dll
    sqldrivers/qsqlibase.dll
    sqldrivers/libmysql.dll
    sqldrivers/libssl-3-x64.dll
    sqldrivers/libcrypto-3-x64.dll
)

set(_qm_total_saved 0)
set(_qm_removed 0)
foreach(_qm_rel IN LISTS _qm_remove_paths)
    set(_qm_full "${QM_DEPLOY_DIR}/${_qm_rel}")
    if(NOT EXISTS "${_qm_full}")
        continue()
    endif()
    _qm_path_size("${_qm_full}" _qm_sz)
    math(EXPR _qm_total_saved "${_qm_total_saved} + ${_qm_sz}")
    math(EXPR _qm_removed "${_qm_removed} + 1")
    if(QM_DRY_RUN)
        message(STATUS "[slim_deployment] (dry-run) ${_qm_rel}  (${_qm_sz} bytes)")
    else()
        file(REMOVE_RECURSE "${_qm_full}")
    endif()
endforeach()

math(EXPR _qm_mb "${_qm_total_saved} / 1048576")
if(QM_DRY_RUN)
    message(STATUS "[slim_deployment] 可删除 ${_qm_removed} 项，约 ${_qm_mb} MB（dry-run）")
else()
    message(STATUS "[slim_deployment] 已删除 ${_qm_removed} 项，约 ${_qm_mb} MB")
endif()