@echo off
:: appname 程序名
:: inputname 输入文件名
:: outputname 输出文件名
:: resultname 程序控制台输出重定向文件名

set appname="StopWait.exe"
set inputname="src\\input.txt"
set outputname="src\\output.txt"
set resultname="src\\result.txt"

for /l %%i in (1,1,10) do (
    echo Test %appname% %%i:
    %appname% > %resultname% 2>&1
    fc /N %inputname% %outputname%
)
pause