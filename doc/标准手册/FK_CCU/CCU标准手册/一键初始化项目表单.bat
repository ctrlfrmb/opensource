@echo off
setlocal enabledelayedexpansion
chcp 65001 > nul

echo === CCU台架表单一键初始化 ===
set /p PROJECT_ID=请输入项目号(如 2026-001): 
set /p PROJECT_NAME=请输入项目名称: 
set /p PRODUCT_ID=请输入产品编号: 
set /p RIG_ID=请输入台架编号: 
set /p PM=请输入项目经理(可空): 
set /p QO=请输入质量负责人(可空): 
set /p CUSTOMER=请输入客户单位(可空): 
set /p ENV=请输入测试环境(可空): 
set /p DUT=请输入DUT版本(可空): 

where python >nul 2>nul
if errorlevel 1 (
  echo [ERROR] 未检测到 Python。请先安装 Python 并确保加入 PATH。
  echo 提示：可在命令行执行 python --version 验证。
  echo.
  pause
  exit /b 1
)

python -c "import openpyxl" >nul 2>nul
if errorlevel 1 (
  echo [ERROR] 检测到 Python，但缺少 openpyxl 依赖。
  echo 请执行：python -m pip install openpyxl
  echo.
  pause
  exit /b 1
)

python "%~dp0init_project_forms.py" ^
  --project-id "%PROJECT_ID%" ^
  --project-name "%PROJECT_NAME%" ^
  --product-id "%PRODUCT_ID%" ^
  --rig-id "%RIG_ID%" ^
  --manager "%PM%" ^
  --quality-owner "%QO%" ^
  --customer "%CUSTOMER%" ^
  --env "%ENV%" ^
  --dut-version "%DUT%"

if errorlevel 1 (
  echo [ERROR] 初始化执行失败，请查看日志：
  echo %~dp0init_project_forms.log
  echo.
  pause
  exit /b 1
)

echo.
echo 初始化完成。详细日志：
echo %~dp0init_project_forms.log
echo 按任意键退出...
pause > nul
