@echo off
set QT_LOGGING_RULES=qt.scenegraph.time.renderloop=true;qt.scenegraph.time.renderer=true;qt.scenegraph.general=true
"build\win-llvm-mingw-release\bin\QueMusic.exe" 2> renderloop.log
