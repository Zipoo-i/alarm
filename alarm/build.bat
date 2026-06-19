@echo off
set CC=gcc
set CFLAGS=-std=c99 -Wall -I.
set LDFLAGS=-lws2_32 -lwevtapi -lole32 -lshell32 -luser32 -lgdi32 -ladvapi32 -lntdll

if not exist CODE\run mkdir CODE\run

cd CODE

echo Compiling alarm_common.c...
%CC% %CFLAGS% -c alarm_common.c

echo Compiling alarm_manager.exe...
%CC% %CFLAGS% -o run\alarm_manager.exe alarm_manager.c alarm_common.o sqlite3.c %LDFLAGS%

echo Compiling trigger.exe...
%CC% %CFLAGS% -o run\trigger.exe trigger.c alarm_common.o sqlite3.c %LDFLAGS%

echo Compiling timer.exe...
%CC% %CFLAGS% -o run\timer.exe timer.c %LDFLAGS%

cd ..

del CODE\alarm_common.o

echo All done. Executables are in 'CODE\run' folder.
pause