@echo off
:: appname ������
:: inputname �����ļ���
:: outputname ����ļ���
:: resultname �������̨����ض����ļ���

set appname="C:\Users\ZPD\Desktop\Networking\lab2\GBN\StopWait\Debug\StopWait.exe"
set inputname="C:\Users\ZPD\Desktop\Networking\lab2\GBN\input.txt"
set outputname="C:\Users\ZPD\Desktop\Networking\lab2\GBN\output.txt"
set resultname="result.txt"

for /l %%i in (1,1,10) do (
    echo Test %appname% %%i:
    %appname% > %resultname% 2>&1
    fc /N %inputname% %outputname%
)
pause