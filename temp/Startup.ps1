#Requires -Version 5.1

$ErrorActionPreference = "Stop"

# ============================================================
# CONFIG
# ============================================================

$RepoUrl = "git@github-personal:ctrlfrmb/common_src.git"

$TargetParentDir = $env:TEMP

$ProjectName = "common_src"

$SourceConfigFile = "D:\opensource\temp\common.pro.user"

$ProjectPath = Join-Path $TargetParentDir $ProjectName

$SshKey = "$HOME\.ssh\id_ed25519_personal"

$SshConfig = "$HOME\.ssh\config"

# ============================================================
# LOG
# ============================================================

function Write-Log {

    param(
        [string]$Message,
        [string]$Color = "White"
    )

    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] $Message" `
        -ForegroundColor $Color
}

Write-Log "========================================" Cyan
Write-Log " Startup Script Begin" Cyan
Write-Log "========================================" Cyan

# ============================================================
# SSH AGENT
# ============================================================

try {

    $svc = Get-Service ssh-agent

    if ($svc.Status -ne "Running") {

        Write-Log "启动 ssh-agent..." Yellow

        Start-Service ssh-agent

        Write-Log "ssh-agent 已启动" Green
    }

} catch {

    Write-Log "ssh-agent 启动失败: $($_.Exception.Message)" Red

    exit 1
}

# ============================================================
# SSH ADD KEY
# ============================================================

try {

    Write-Log "加载 SSH Key..." Cyan

    & ssh-add $SshKey | Out-Null

    if ($LASTEXITCODE -ne 0) {

        throw "ssh-add failed"
    }

    Write-Log "SSH Key 加载成功" Green

} catch {

    Write-Log "SSH Key 加载失败" Red

    exit 1
}

# ============================================================
# SSH TEST
# ============================================================

try {

    Write-Log "测试 GitHub SSH 连接..." Cyan

    & ssh `
        -F $SshConfig `
        -o StrictHostKeyChecking=no `
        -T git@github-personal

    Write-Log "GitHub SSH 测试完成" Green

} catch {

    Write-Log "GitHub SSH 测试失败" Red

    exit 1
}

# ============================================================
# CHECK OLD PROJECT
# ============================================================

if (Test-Path $ProjectPath) {

    Write-Log "发现旧目录: $ProjectPath" Yellow

    $NeedBackup = $false

    $Reason = ""

    try {

        Set-Location $ProjectPath

        if (-not (Test-Path ".git")) {

            $NeedBackup = $true

            $Reason = "不是 git repo"
        }
        else {

            $status = & git status --porcelain

            if ($status) {

                $NeedBackup = $true

                $Reason = "存在未提交代码"
            }
        }

    } catch {

        $NeedBackup = $true

        $Reason = $_.Exception.Message

    } finally {

        Set-Location $TargetParentDir
    }

    if ($NeedBackup) {

        $BackupName = "${ProjectName}_Backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"

        $BackupPath = Join-Path $TargetParentDir $BackupName

        Write-Log "执行备份: $Reason" Yellow

        Rename-Item `
            -Path $ProjectPath `
            -NewName $BackupName

        Write-Log "备份完成: $BackupPath" Green

    } else {

        Write-Log "旧目录干净，删除..." Gray

        Remove-Item `
            -Path $ProjectPath `
            -Recurse `
            -Force
    }
}

# ============================================================
# GIT CLONE
# ============================================================

try {

    Write-Log "开始 Clone..." Cyan

    $env:GIT_SSH_COMMAND = "ssh -F `"$SshConfig`""

    & git clone `
        --depth 1 `
        $RepoUrl `
        $ProjectPath

    if ($LASTEXITCODE -ne 0) {

        throw "git clone failed"
    }

    Write-Log "Clone 成功" Green

} catch {

    Write-Log "Clone 失败: $($_.Exception.Message)" Red

    exit 1
}

# ============================================================
# COPY CONFIG
# ============================================================

if (Test-Path $SourceConfigFile) {

    try {

        Copy-Item `
            $SourceConfigFile `
            $ProjectPath `
            -Force

        Write-Log "Qt 配置恢复完成" Green

    } catch {

        Write-Log "Qt 配置恢复失败" Red
    }
}

# ============================================================
# OPEN
# ============================================================

Write-Log "打开项目目录..." Cyan

Invoke-Item $ProjectPath

Write-Log "========================================" Cyan
Write-Log " DONE" Green
Write-Log "========================================" Cyan