@echo off
rem ============================================================
rem QueMusic 收藏写库诊断  —— 双击运行即可，无需管理员
rem 结果会打印在窗口里，同时写到 桌面\qm-dbcheck.txt
rem ============================================================
set OUT=%USERPROFILE%\Desktop\qm-dbcheck.txt
echo QueMusic 数据库写权限诊断 > "%OUT%"
echo 时间: %DATE% %TIME% >> "%OUT%"
echo. >> "%OUT%"

set DB=%APPDATA%\BroNekoX\QueMusic\player_data.db

echo [1] 数据库文件 >> "%OUT%"
if exist "%DB%" (
    echo    存在: %DB% >> "%OUT%"
    dir "%DB%" | findstr player_data >> "%OUT%"
) else (
    echo    不存在: %DB% >> "%OUT%"
)
echo. >> "%OUT%"

echo [2] 尝试删除 -wal / -shm（SQLite 残留锁最常见的元凶） >> "%OUT%"
if exist "%DB%-wal" ( del /f /q "%DB%-wal" && echo    已删除 -wal >> "%OUT%" ) else ( echo    无 -wal >> "%OUT%" )
if exist "%DB%-shm" ( del /f /q "%DB%-shm" && echo    已删除 -shm >> "%OUT%" ) else ( echo    无 -shm >> "%OUT%" )
echo. >> "%OUT%"

echo [3] 目录里能否新建文件 >> "%OUT%"
echo test > "%APPDATA%\BroNekoX\QueMusic\writetest.tmp" 2>>"%OUT%" && (
    echo    可以新建文件 >> "%OUT%"
    del /f /q "%APPDATA%\BroNekoX\QueMusic\writetest.tmp"
) || echo    无法新建文件 >> "%OUT%"
echo. >> "%OUT%"

echo [4] 用 sqlite3 直接写库（如果装了） >> "%OUT%"
where sqlite3 >nul 2>>"%OUT%" && (
    sqlite3 "%DB%" "insert or replace into favorites (id,title,artist,cover,source,duration,type) values ('__check__','c','c','',1,0,'song'); select count(*) from favorites;" >> "%OUT%" 2>&1
    sqlite3 "%DB%" "delete from favorites where id='__check__';" >> "%OUT%" 2>&1
) || echo    未安装 sqlite3，跳过 >> "%OUT%"
echo. >> "%OUT%"

echo [5] 是否被进程占用 >> "%OUT%"
tasklist | findstr /i "QueMusic" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo 完成。请把桌面上的 qm-dbcheck.txt 内容发出来。
echo 完成。请把桌面上的 qm-dbcheck.txt 内容发出来。 >> "%OUT%"
type "%OUT%"
pause
