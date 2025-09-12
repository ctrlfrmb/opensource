# Startup.ps1 - 克隆代码仓库并准备开发环境 (最终健壮版)

# --- 可配置区域 ---
$RepoUrl = "git@github.com:ctrlfrmb/common_src.git"
$TargetParentDir = $env:TEMP 
$ProjectName = "common_src"
$SourceConfigFile = "D:\opensource\temp\common.pro.user" 
# --- 可配置区域结束 ---

# 拼接出完整的项目路径
$ProjectPath = Join-Path -Path $TargetParentDir -ChildPath $ProjectName

# --- 脚本核心逻辑 ---
Write-Host "--- 启动脚本开始执行 @ $(Get-Date) ---"

# 检查目标文件夹是否存在，如果存在，先强制删除
if (Test-Path $ProjectPath) {
    Write-Host "发现旧的目录: $ProjectPath，正在强制删除..."
    try {
        Remove-Item -Path $ProjectPath -Recurse -Force -ErrorAction Stop
        Write-Host "旧目录删除成功。" -ForegroundColor Green
    } catch {
        Write-Host "删除旧目录失败: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
}

# 启动 ssh-agent 并添加密钥
try {
    Write-Host "正在启动 SSH-Agent 并加载密钥..."
    if ((Get-Service -Name ssh-agent).Status -ne 'Running') {
        Start-Service ssh-agent -ErrorAction Stop
    }
    
    # 执行 ssh-add 并检查退出代码
    ssh-add
    if ($LASTEXITCODE -ne 0) {
        # 手动抛出异常，以便被 catch 捕获
        throw "ssh-add 命令执行失败，请检查 SSH 密钥配置。"
    }
    
    Write-Host "SSH 密钥加载成功。" -ForegroundColor Green
} catch {
    Write-Host "启动 SSH-Agent 或加载密钥失败: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "提示：请确保 OpenSSH Client 功能已安装，并以管理员身份运行过 'Set-Service -Name ssh-agent -StartupType Automatic'" -ForegroundColor Yellow
    exit 1
}

# 执行 git clone
try {
    Write-Host "正在从 $RepoUrl 克隆项目到 $ProjectPath ..."
    
    # 执行 git clone 并检查退出代码
    git clone --depth 1 $RepoUrl $ProjectPath -q
    if ($LASTEXITCODE -ne 0) {
        throw "git clone 命令执行失败。请检查网络、Git仓库地址和SSH权限。"
    }

    Write-Host "项目克隆成功！" -ForegroundColor Green
    Write-Host "项目路径: $ProjectPath"

    # --- 拷贝配置文件 ---
    Write-Host "正在拷贝配置文件..."
    if (Test-Path $SourceConfigFile) {
        # Copy-Item 是 PowerShell 内置命令，可以用 -ErrorAction Stop
        Copy-Item -Path $SourceConfigFile -Destination $ProjectPath -ErrorAction Stop
        Write-Host "配置文件 '$SourceConfigFile' 拷贝成功！" -ForegroundColor Green
    } else {
        Write-Host "警告：源配置文件 '$SourceConfigFile' 不存在，跳过拷贝。" -ForegroundColor Yellow
    }

} catch {
    Write-Host "Git 克隆或文件拷贝过程中发生错误: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host "--- 启动脚本执行完毕 ---"

# 打开项目文件夹
Write-Host "正在打开项目文件夹..."
explorer.exe $ProjectPath
