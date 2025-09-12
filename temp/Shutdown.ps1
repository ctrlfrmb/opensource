# Shutdown.ps1 - 自动提交代码并清理项目文件夹

# --- 可配置区域 (请与 Startup.ps1 保持一致) ---
$TargetParentDir = $env:TEMP
$ProjectName = "common_src"
$CommitMessage = "feat: Auto-commit on shutdown" # 自动提交时使用的消息
# --- 可配置区域结束 ---

# 拼接出完整的项目路径
$ProjectPath = Join-Path -Path $TargetParentDir -ChildPath $ProjectName

# --- 脚本核心逻辑 ---
Write-Host "--- 清理与提交脚本开始执行 @ $(Get-Date) ---"

# 1. 检查项目目录是否存在
if (-not (Test-Path $ProjectPath)) {
    Write-Host "项目目录 '$ProjectPath' 不存在，无需提交或清理。" -ForegroundColor Yellow
    Write-Host "--- 脚本执行完毕 ---"
    exit 0 # 正常退出
}

# --- 自动 Git 提交逻辑 ---
try {
    Write-Host "正在进入项目目录: $ProjectPath"
    Set-Location -Path $ProjectPath # 切换当前目录到项目目录，这是执行git命令的前提

    # 确保 ssh-agent 正在运行，为 git push 做准备
    Write-Host "正在检查 SSH-Agent..."
    if ((Get-Service -Name ssh-agent).Status -ne 'Running') {
        Start-Service ssh-agent
    }
    ssh-add
    if ($LASTEXITCODE -ne 0) { throw "加载 SSH 密钥失败。" }
    Write-Host "SSH 密钥已加载。" -ForegroundColor Green

    # 检查工作区是否有变动
    # git status --porcelain 会在有变动时输出内容，无变动时无输出
    $gitStatus = git status --porcelain
    if ($gitStatus) {
        Write-Host "检测到代码变更，正在执行自动提交..." -ForegroundColor Cyan
        
        # 执行 Git 操作
        Write-Host "Step 1: git add ."
        git add .
        if ($LASTEXITCODE -ne 0) { throw "git add . 执行失败。" }

        Write-Host "Step 2: git commit"
        git commit -m $CommitMessage
        if ($LASTEXITCODE -ne 0) { throw "git commit 执行失败。" }

        Write-Host "Step 3: git push"
        git push
        if ($LASTEXITCODE -ne 0) {
            # push 失败只警告，不中断脚本，因为代码已本地提交，后续清理仍需执行
            Write-Host "警告: git push 失败！代码已本地提交，但未推送到远程仓库。" -ForegroundColor Yellow
        } else {
            Write-Host "代码已成功推送到远程仓库！" -ForegroundColor Green
        }

    } else {
        Write-Host "代码库无变更，无需提交。" -ForegroundColor Green
    }
} catch {
    Write-Host "Git 自动提交过程中发生严重错误: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "尽管提交失败，仍将继续执行清理操作。" -ForegroundColor Yellow
}

# --- 清理项目文件夹逻辑 ---
Write-Host "正在执行项目文件夹清理操作..."
try {
    # 切换回原来的目录，避免因占用导致删除失败
    Set-Location $env:TEMP
    Remove-Item -Path $ProjectPath -Recurse -Force -ErrorAction Stop
    Write-Host "项目目录 '$ProjectPath' 删除成功！" -ForegroundColor Green
} catch {
    Write-Host "删除目录失败: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "--- 清理与提交脚本执行完毕 ---"
