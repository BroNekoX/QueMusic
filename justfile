# 检测操作系统并自动选择预设
os := `uname -s`
preset := if os == "Darwin" { "mac-clang-release" } else if os == "Linux" { "linux-gcc-release" } else { "win-llvm-mingw-release" }

# 默认构建命令：just b
b:
    cmake --preset {{preset}}
    cmake --build build/{{preset}} -j 8

# 运行程序：just r
r: b
    ./build/{{preset}}/bin/QueMusic.exe   # Windows 可执行文件在 bin 目录下[reference:2]，且带 .exe 后缀

# Mac专用：打包成 .app 方便分发
mac-bundle: b
    macdeployqt ./build/{{preset}}/QueMusic.app -dmg
    @echo "打包完成，DMG文件在 build/ 目录下"

# 清理编译文件
clean:
    rm -rf build/