# Startup.ps1 - 安全克隆代码并准备环境
# 功能：检查旧目录状态 -> (有未提交代码? 备份 : 删除) -> 克隆新代码 -> 拷贝配置

# --- 可配置区域 ---
$RepoUrl = "git@github.com:ctrlfrmb/common_src.git"
$TargetParentDir = $env:TEMP 
$ProjectName = "common_src"
$SourceConfigFile = "D:\opensource\temp\common.pro.user" 
# --- 可配置区域结束 ---

$ProjectPath = Join-Path -Path $TargetParentDir -ChildPath $ProjectName

function Write-Log {
    param([string]$Message, [string]$Color = "White")
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] $Message" -ForegroundColor $Color
}

Write-Log "--- 启动脚本开始执行 ---" "Cyan"

# ==========================================
# 1. 启动 SSH Agent (前置，确保 Git 可用)
# ==========================================
try {
    if ((Get-Service -Name ssh-agent).Status -ne 'Running') {
        Start-Service ssh-agent -ErrorAction Stop
        Write-Log "SSH Agent 已启动" "Gray"
    }
    
    # 尝试添加默认密钥，忽略错误输出（避免红字吓人），通过退出码判断
    ssh-add 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Log "警告：SSH 密钥加载可能未完成，请确认密钥已配置。" "Yellow"
    } else {
        Write-Log "SSH 密钥加载成功。" "Green"
    }
} catch {
    Write-Log "SSH 服务启动失败: $($_.Exception.Message)" "Red"
    exit 1
}

# ==========================================
# 2. 智能检查旧目录 (核心安全逻辑)
# ==========================================
if (Test-Path $ProjectPath) {
    Write-Log "检测到旧项目目录: $ProjectPath" "Gray"
    
    $NeedBackup = $false
    $Reason = ""

    # 尝试进入目录检查 Git 状态
    try {
        Set-Location $ProjectPath -ErrorAction Stop
        
        if (-not (Test-Path ".git")) {
            $NeedBackup = $true
            $Reason = "目录存在但不是 Git 仓库，可能是残留文件"
        } else {
            # 检查1: 是否有未提交的变更 (包括 untracked)
            $gitStatus = git status --porcelain
            if ($gitStatus) {
                $NeedBackup = $true
                $Reason = "发现未提交的本地变更"
            } 
            # 检查2: 是否有已提交但未推送的 Commit
            # git cherry -v 会列出本地有但远程没有的 commit
            elseif (git cherry -v) {
                $NeedBackup = $true
                $Reason = "发现已 Commit 但未 Push 的代码"
            }
        }
    } catch {
        $NeedBackup = $true
        $Reason = "无法访问目录或执行 Git 命令 ($($_.Exception.Message))"
    } finally {
        # 务必切回上级目录，否则无法重命名/删除文件夹
        Set-Location $TargetParentDir
    }

    # 根据检查结果决定动作
    if ($NeedBackup) {
        $BackupName = "${ProjectName}_Backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
        $BackupPath = Join-Path $TargetParentDir $BackupName
        
        Write-Log "!!! 安全警告 !!!" "Red"
        Write-Log "原因: $Reason" "Yellow"
        Write-Log "正在执行备份，而不是删除..." "Yellow"
        
        try {
            Rename-Item -Path $ProjectPath -NewName $BackupName -ErrorAction Stop
            Write-Log "已备份至: $BackupPath" "Green"
        } catch {
            Write-Log "备份失败，脚本终止！请手动处理: $ProjectPath" "Red"
            exit 1
        }
    } else {
        Write-Log "旧目录代码已同步（干净），正在删除..." "Gray"
        try {
            Remove-Item -Path $ProjectPath -Recurse -Force -ErrorAction Stop
        } catch {
            Write-Log "删除失败: $($_.Exception.Message)" "Red"
            exit 1
        }
    }
}

# ==========================================
# 3. 克隆新代码
# ==========================================
try {
    Write-Log "正在克隆代码..." "Cyan"
    # --depth 1 浅克隆加快速度
    git clone --depth 1 $RepoUrl $ProjectPath -q
    
    if ($LASTEXITCODE -ne 0) {
        throw "Git Clone 失败，请检查网络或权限"
    }
    Write-Log "克隆成功！" "Green"
} catch {
    Write-Log "错误: $($_.Exception.Message)" "Red"
    exit 1
}

# ==========================================
# 4. 恢复开发环境配置
# ==========================================
if (Test-Path $SourceConfigFile) {
    try {
        Copy-Item -Path $SourceConfigFile -Destination $ProjectPath -Force -ErrorAction Stop
        Write-Log "配置文件已恢复 (.pro.user)" "Green"
    } catch {
        Write-Log "配置文件拷贝失败: $($_.Exception.Message)" "Red"
    }
} else {
    Write-Log "未找到源配置文件，跳过配置恢复" "Yellow"
}

# ==========================================
# 5. 打开工作区
# ==========================================
Write-Log "任务完成，正在打开文件夹..." "Cyan"
Invoke-Item $ProjectPath
