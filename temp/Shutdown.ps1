# Shutdown.ps1 - 自动提交代码并清理项目文件夹（修复版）

# --- 可配置区域 ---
$TargetParentDir = $env:TEMP
$ProjectName = "common_src"
$CommitMessage = "feat: Auto-commit on shutdown $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
# --- 可配置结束 ---

$ScriptDir = $PSScriptRoot
if (-not $ScriptDir) { $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
$LogFile = Join-Path -Path $ScriptDir -ChildPath "backup/AutoCommit_Logoff.log"
$ProjectPath = Join-Path -Path $TargetParentDir -ChildPath $ProjectName

# 标志位：只有当所有步骤完美成功时，才允许删除代码
$SafeToCleanup = $false 

function Write-Log {
    param([string]$Message, [string]$Color = "White")
    $TimeStamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $LogLine = "[$TimeStamp] $Message"
    Write-Host $LogLine -ForegroundColor $Color
    try { Add-Content -Path $LogFile -Value $LogLine -Encoding UTF8 -ErrorAction SilentlyContinue } catch {}
}

Write-Log "==========================================" "Gray"
Write-Log "--- Auto Commit & Cleanup 脚本开始执行 ---" "Cyan"

if (-not (Test-Path $ProjectPath)) {
    Write-Log "项目目录不存在，跳过。" "Yellow"
    exit 0
}

try {
    Write-Log "进入项目目录: $ProjectPath"
    Set-Location -Path $ProjectPath -ErrorAction Stop

    # 1. 启动 SSH
    if ((Get-Service ssh-agent -ErrorAction SilentlyContinue).Status -ne 'Running') {
        Start-Service ssh-agent -ErrorAction SilentlyContinue
    }
    ssh-add 2>$null

    # 2. 检查变更
    if (-not (git status --porcelain)) {
        Write-Log "无代码变更，无需提交。" "Green"
        $SafeToCleanup = $true # 无变更也视为安全，可以清理
    } else {
        Write-Log "检测到变更，开始本地提交..." "Cyan"
        
        git add . 2>&1 | Out-Null
        git commit -m $CommitMessage 2>&1 | Out-Null
        
        if ($LASTEXITCODE -ne 0) { throw "Git commit 失败" }

        Write-Log "本地 Commit 成功，开始后台 Push..."

        # 3. 修复：Start-Job 需要手动设置工作目录，并返回退出代码
        $pushJob = Start-Job -ScriptBlock { 
            param($WorkDir) 
            Set-Location -Path $WorkDir  # 必须在 Job 内部切换目录
            
            # 执行 push 并捕获输出
            $output = git push 2>&1 
            
            # 返回一个对象，包含输出内容和退出码
            [PSCustomObject]@{
                StdOut = $output
                ExitCode = $LASTEXITCODE
            }
        } -ArgumentList $ProjectPath

        # 等待 Job 完成
        if (Wait-Job $pushJob -Timeout 20) {
            $result = Receive-Job $pushJob
            
            # 输出 Job 内部的日志
            if ($result.StdOut) { 
                $result.StdOut | ForEach-Object { Write-Log "  [Git] $_" "Gray" } 
            }

            # 4. 修复：根据 Job 内部传回的 ExitCode 判断成功
            if ($result.ExitCode -eq 0) {
                Write-Log "Git Push 成功！" "Green"
                $SafeToCleanup = $true
            } else {
                Write-Log "Git Push 失败 (ExitCode: $($result.ExitCode))" "Red"
                $SafeToCleanup = $false
            }
        } else {
            Write-Log "Git Push 超时，强制停止" "Red"
            Stop-Job $pushJob
            $SafeToCleanup = $false
        }
        
        Remove-Job $pushJob -Force
    }
}
catch {
    Write-Log "发生异常: $($_.Exception.Message)" "Red"
    $SafeToCleanup = $false
}

# 5. 修复：根据标志位决定是否删除
Write-Log "准备清理..."
if ($SafeToCleanup) {
    try {
        Set-Location $env:TEMP
        Remove-Item -Path $ProjectPath -Recurse -Force -ErrorAction Stop
        Write-Log "清理成功：项目目录已删除。" "Green"
    } catch {
        Write-Log "删除目录失败: $($_.Exception.Message)" "Red"
    }
} else {
    Write-Log "警告！！由于 Push 失败或发生错误，保留本地目录以防丢失数据！" "Red"
    Write-Log "目录位置: $ProjectPath" "Red"
    # 可以选择重命名该目录作为备份
    # Rename-Item $ProjectPath "$ProjectPath-Backup-$(Get-Date -Format 'yyyyMMdd-HHmmss')"
}

Write-Log "--- 脚本结束 ---" "Cyan"
