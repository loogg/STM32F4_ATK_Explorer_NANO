@echo off
@REM AUTHOR: WKJay
@REM VERSION: V1.0.1

@REM 请根据实际情况修改以下变量，注意如果使用相对路径，需要以KEIL的工程文件路径为基准，而非本脚本所在路径

@REM KEIL 设置中的输出文件名
set EXEC_NAME=STM32F4_Explorer_NANO
@REM KEIL 设置中的输出文件路径
set OBJ_PATH=.\build\Obj
@REM 输出文件路径
set OUTPUT_PATH=..\Output
@REM 包含版本字符串文件的路径
set VERSION_FILE_PATH=..\Applications\system.h
@REM fromelf.exe 的路径（KEIL安装目录）
set FROMELF_PATH=D:\Keil_v5\ARM\ARMCC\bin
@REM 版本字符串的格式
set VERSION_PATTERN="#define SOFTWARE_VERSION"

@REM 获取日期【这里的日期格式为 YYYY-MM-HH hh:mm:ss，不同时区或配置下格式可能不同，请自行调整】
set YEAR=%DATE:~2,2%
set MONTH=%DATE:~5,2%
set DAY=%DATE:~8,2%

@REM 在时间字符串出现空格时补0，避免后续处理出错
set _TIME=%TIME: =0%
set HOUR=%_TIME:~0,2%
set MINUTE=%_TIME:~3,2%
set SECOND=%_TIME:~6,2%
set CURRENT_DATE=%YEAR%%MONTH%%DAY%_%HOUR%%MINUTE%%SECOND%
@REM echo "Current date: %CURRENT_DATE%"

for /f "tokens=3 delims= " %%i in ('findstr /C:%VERSION_PATTERN% %VERSION_FILE_PATH%') do set sw_ver=%%i
@REM 去除字符串两端的双引号，如果代码中定义版本字符的不包含双引号，需要去除此行
set sw_ver=%sw_ver:~1,-1%


set output_file_name=%EXEC_NAME%

if not exist %OUTPUT_PATH% mkdir %OUTPUT_PATH%
echo "Clean old files..."
for /f "delims=" %%A in ('dir /b %OUTPUT_PATH%') do del %OUTPUT_PATH%\%%A"

set output_file_name=%output_file_name%_V%sw_ver%_%CURRENT_DATE%

@REM GET BIN
echo "Output bin file: %OUTPUT_PATH%\%output_file_name%.bin"
%FROMELF_PATH%\fromelf --bin %OBJ_PATH%\%EXEC_NAME%.axf --output %OUTPUT_PATH%\%output_file_name%.bin

@REM GET HEX
echo "Output hex file: %OUTPUT_PATH%\%output_file_name%.hex"
copy %OBJ_PATH%\%EXEC_NAME%.hex %OUTPUT_PATH%\%output_file_name%.hex

@REM GET DIS
echo "Output dis file: %OUTPUT_PATH%\%output_file_name%.dis"
%FROMELF_PATH%\fromelf --text -a -c --output=%OUTPUT_PATH%\%output_file_name%_disassembly.txt %OBJ_PATH%\%EXEC_NAME%.axf

exit
