# qmltc（QML 类型编译器）接入 —— 实验性，默认关闭
# =============================================================================
# 由 CMakeLists.txt 通过 include() 引入，受 QUEMUSIC_ENABLE_QMLTC 控制（默认 OFF）。
#
# 以下结论全部来自在本机 Qt 6.10.3（开源版）上的实测：
#
#  1. qmltc 在开源 Qt 中是可用的（qmltc.exe 随 Qt 一起发布）；
#     商业版独有的是 qmlsc（用于把 JS 函数/绑定进一步编译为 C++），本机 bin 下无 qmlsc.exe。
#
#  2. qmltc 只能编译"文档中所有元素类型都属于本模块、或属于只暴露 C++ 类的模块"的 QML。
#     一旦用到其它模块里由 QML 定义的类型（QtQuick.Controls.*、QtQuick.Dialogs、
#     Qt5Compat.GraphicalEffects 等）就会报错，例如：
#       Can't compile the QML base type "TextField" to C++ because it lives in
#       "QtQuick.Controls.Basic" instead of the current file's "QueMusic" QML module.
#     本工程 78 个 QML 文件中，56 个直接依赖 QtQuick.Controls 系模块（整个 UI 基于 Controls）。
#     （QtQuick.Effects / QtQuick.Layouts / QtQuick.Shapes 是纯 C++ 模块，不构成阻塞。）
#
#  3. 被编译文档引用到的本模块其它 QML 类型必须一并编译（生成的 C++ 类会互相继承），
#     所以白名单必须是"闭包"：白名单文件不允许引用被跳过的文件。
#
#  4. 【关键】qmltc 无法处理位于模块"子目录"中的文档。
#     实测：把引用方文件放进子目录后，连同模块的 QML 类型与 C++ QML_ELEMENT 类型都会
#     解析失败（报 lives in ""），而放在模块根目录时一切正常。
#     本工程可编译的那批组件全部位于 components/ 与 centers/ 下，因此当前布局下
#     qmltc 无法生效。
#
#  5. 运行时收益只在 C++ 侧直接实例化生成的类时出现：
#     QQmlComponent / engine.load() / Loader { source: ... } 加载文档时依然走 QML 源码。
#     实测加载被编译文档得到的对象类名为 <File>_QMLTYPE_n，而非 <命名空间>::<File>；
#     只有 new QueMusic::<File>(&engine, ...) 才真正省掉 QML 解释开销。
#
# 结论：在本工程当前结构下开启 qmltc 既不会带来运行性能提升，也无法通过构建。
# 因此这里做了安全降级：只要白名单里还存在子目录文件，就打印说明并保持关闭，
# 不会让构建失败。等 UI 摆脱 QtQuick.Controls 依赖、并把目标组件挪到模块根目录后，
# 把它加入下方白名单即可自动启用。
# =============================================================================

if(NOT QUEMUSIC_ENABLE_QMLTC)
    return()
endif()

# 期望可被 qmltc 编译的组件（仅使用 QtQuick/QtQml + 本模块内这批文件，构成自洽闭包）
set(QUE_MUSIC_QMLTC_ALLOWED
    components/Style.qml
    components/StyleThemes.qml
    components/StyleSettings.qml
    components/Options.qml
    components/OptionsSettings.qml
    components/OptionsLastSongs.qml
    components/OptionsShortCuts.qml
    components/Playback.qml
    components/PicHeadCard.qml
    components/QBigDrop.qml
    components/QCard.qml
    components/QCardDrop.qml
    components/QContentCard.qml
    components/QFloatCard.qml
    components/QHead.qml
    components/QLoadBar.qml
    components/QLoadSign.qml
    components/QPages.qml
    components/QRCodeView.qml
    components/QSortModel.qml
    components/QSwitch.qml
    components/QWideDrop.qml
    components/SettingItem.qml
    components/SettingItemCard.qml
    centers/CenterList.qml
    centers/CenterQueue.qml
    centers/CenterTabs.qml
)

# 目前这些文件都在子目录中 —— qmltc 无法编译（见上文第 4 条）
set(_qmltc_subdir_files)
foreach(qml_file IN LISTS QUE_MUSIC_QMLTC_ALLOWED)
    get_filename_component(_qmltc_file_dir "${qml_file}" DIRECTORY)
    if(NOT _qmltc_file_dir STREQUAL "")
        list(APPEND _qmltc_subdir_files "${qml_file}")
    endif()
endforeach()

if(_qmltc_subdir_files)
    message(WARNING
        "QUEMUSIC_ENABLE_QMLTC=ON，但以下文件位于 QML 模块的子目录中，"
        "qmltc（Qt 6.10）无法编译它们，本次不会启用类型编译：\n  ${_qmltc_subdir_files}\n"
        "把它们移动到 QML 模块根目录（本工程即仓库根）后再重新配置即可启用；"
        "详见 cmake/qmltc.cmake 顶部说明。")
    return()
endif()

set(QUE_MUSIC_QMLTC_ACTIVE TRUE)

# qmltc 生成代码会 include Qt 私有头文件，必须链接对应 Private 模块
find_package(Qt6 REQUIRED COMPONENTS QmlPrivate QuickPrivate)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::QmlPrivate Qt6::QuickPrivate)

# 白名单之外的文件显式跳过，避免 qmltc 构建失败
foreach(qml_file IN LISTS QUE_MUSIC_QML_FILES)
    if(NOT qml_file MATCHES "\\.qml$")
        continue()
    endif()
    list(FIND QUE_MUSIC_QMLTC_ALLOWED "${qml_file}" _qmltc_allowed_index)
    if(_qmltc_allowed_index EQUAL -1)
        set_source_files_properties("${qml_file}" PROPERTIES QT_QML_SKIP_TYPE_COMPILER TRUE)
    endif()
endforeach()

message(STATUS "QueMusic: qmltc 已启用（白名单 ${QUE_MUSIC_QMLTC_ALLOWED}）")
