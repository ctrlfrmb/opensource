# Shutdown.ps1 - 自动提交代码并清理项目文件夹（最终优化版）

# --- 可配置区域 ---
$TargetParentDir = $env:TEMP                  # 项目放在临时目录
$ProjectName = "common_src"                   # 项目文件夹名
$CommitMessage = "feat: Auto-commit on shutdown $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
# --- 可配置结束 ---

# 自动获取当前脚本所在目录（无论从哪里调用，都能正确找到）
$ScriptDir = $PSScriptRoot
if (-not $ScriptDir) {
    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}

# 日志文件放在脚本同目录，持久保存
$LogFile = Join-Path -Path $ScriptDir -ChildPath "backup/AutoCommit_Logoff.log"

$ProjectPath = Join-Path -Path $TargetParentDir -ChildPath $ProjectName

# 日志函数（同时输出到控制台和文件）
function Write-Log {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    $TimeStamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $LogLine = "[$TimeStamp] $Message"
    Write-Host $LogLine -ForegroundColor $Color
    Add-Content -Path $LogFile -Value $LogLine -Encoding UTF8
}

Write-Log "==========================================" "Gray"
Write-Log "--- Auto Commit & Cleanup 脚本开始执行 ---" "Cyan"
Write-Log "脚本路径: $PSCommandPath" "Gray"

# 1. 检查项目目录是否存在
if (-not (Test-Path $ProjectPath)) {
    Write-Log "项目目录 '$ProjectPath' 不存在，无需操作。" "Yellow"
    Write-Log "--- 脚本执行结束 ---" "Cyan"
    exit 0
}

try {
    Write-Log "进入项目目录: $ProjectPath"
    Set-Location -Path $ProjectPath -ErrorAction Stop

    # 启动 ssh-agent 并加载密钥（如果需要）
    if ((Get-Service ssh-agent -ErrorAction SilentlyContinue).Status -ne 'Running') {
        Start-Service ssh-agent -ErrorAction SilentlyContinue
        Write-Log "ssh-agent 已启动"
    }
    ssh-add 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Log "警告：ssh-add 失败（可能无密钥或需手动输入）" "Yellow"
    } else {
        Write-Log "SSH 密钥加载成功" "Green"
    }

    # 检查是否有变更
    $status = git status --porcelain
    if (-not $status) {
        Write-Log "无代码变更，跳过提交" "Green"
    } else {
        Write-Log "检测到变更，开始自动提交..." "Cyan"

        git add . 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "git add 失败" }

        git commit -m $CommitMessage 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "git commit 失败" }

        Write-Log "本地 commit 成功，开始 push（15秒超时）..."
        $pushJob = Start-Job -ScriptBlock { git push }
        if (Wait-Job $pushJob -Timeout 15) {
            $pushOutput = Receive-Job $pushJob
            if ($pushJob.State -eq 'Completed' -and $LASTEXITCODE -eq 0) {
                Write-Log "git push 成功！" "Green"
            } else {
                Write-Log "git push 失败（代码已本地提交）" "Yellow"
                Write-Log $pushOutput
            }
        } else {
            Write-Log "git push 超时（>15秒），已终止，代码已本地提交" "Yellow"
            Stop-Job $pushJob
            Remove-Job $pushJob
        }
    }
}
catch {
    Write-Log "Git 操作出错: $($_.Exception.Message)" "Red"
    Write-Log "错误详情: $($_.ScriptStackTrace)" "Red"
    Write-Log "将继续执行清理操作" "Yellow"
}

# 清理项目目录
try {
    Set-Location $env:TEMP -ErrorAction SilentlyContinue
    Remove-Item -Path $ProjectPath -Recurse -Force -ErrorAction Stop
    Write-Log "项目目录 '$ProjectPath' 已成功删除！" "Green"
}
catch {
    Write-Log "删除项目目录失败: $($_.Exception.Message)" "Red"
}

Write-Log "--- 脚本执行完成 ---" "Cyan"
Write-Log "==========================================" "Gray"
