#Requires -Version 5.1

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "======================================="
Write-Host " Git Multi Account Setup"
Write-Host "======================================="
Write-Host ""

$sshDir = "$HOME\.ssh"

if (!(Test-Path $sshDir)) {
    New-Item -ItemType Directory -Path $sshDir | Out-Null
}

# --------------------------------------------------
# SSH KEYS
# --------------------------------------------------

$personalKey = "$sshDir\id_ed25519_personal"
$workKey = "$sshDir\id_ed25519_work"

if (!(Test-Path $personalKey)) {

    Write-Host "[STEP] Generate PERSONAL ssh key..."

    ssh-keygen `
        -t ed25519 `
        -C "ctrlfrmb@vip.qq.com" `
        -f $personalKey `
        -N '""'
}

if (!(Test-Path $workKey)) {

    Write-Host "[STEP] Generate WORK ssh key..."

    ssh-keygen `
        -t ed25519 `
        -C "ctrlfrmb@gmail.com" `
        -f $workKey `
        -N '""'
}

# --------------------------------------------------
# SSH CONFIG
# --------------------------------------------------

$configPath = "$sshDir\config"

$configContent = @"

# PERSONAL
Host github-personal
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_personal

# WORK
Host github-work
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_work

"@

Set-Content `
    -Path $configPath `
    -Value $configContent `
    -Encoding UTF8

Write-Host "[INFO] SSH config updated."

# --------------------------------------------------
# GITCONFIG
# --------------------------------------------------

$mainGitConfig = "$HOME\.gitconfig"

$mainGitConfigContent = @"

[includeIf "gitdir:C:/Users/80139/source/"]
    path = ~/.gitconfig-personal

[includeIf "gitdir:D:/source/"]
    path = ~/.gitconfig-work

"@

Set-Content `
    -Path $mainGitConfig `
    -Value $mainGitConfigContent `
    -Encoding UTF8

Write-Host "[INFO] ~/.gitconfig updated."

# --------------------------------------------------
# PERSONAL CONFIG
# --------------------------------------------------

$personalGitConfig = "$HOME\.gitconfig-personal"

$personalContent = @"

[user]
    name = ctrlfrmb
    email = ctrlfrmb@vip.qq.com

[core]
    sshCommand = ssh -i ~/.ssh/id_ed25519_personal

"@

Set-Content `
    -Path $personalGitConfig `
    -Value $personalContent `
    -Encoding UTF8

Write-Host "[INFO] ~/.gitconfig-personal updated."

# --------------------------------------------------
# WORK CONFIG
# --------------------------------------------------

$workGitConfig = "$HOME\.gitconfig-work"

$workContent = @"

[user]
    name = ctrlfrmb
    email = ctrlfrmb@gmail.com

[core]
    sshCommand = ssh -i ~/.ssh/id_ed25519_work

"@

Set-Content `
    -Path $workGitConfig `
    -Value $workContent `
    -Encoding UTF8

Write-Host "[INFO] ~/.gitconfig-work updated."

# --------------------------------------------------
# SHOW PUBLIC KEYS
# --------------------------------------------------

Write-Host ""
Write-Host "======================================="
Write-Host " PERSONAL PUBLIC KEY"
Write-Host "======================================="
Write-Host ""

Get-Content "$personalKey.pub"

Write-Host ""
Write-Host "======================================="
Write-Host " WORK PUBLIC KEY"
Write-Host "======================================="
Write-Host ""

Get-Content "$workKey.pub"

Write-Host ""
Write-Host "======================================="
Write-Host " DONE"
Write-Host "======================================="
Write-Host ""

Write-Host "NEXT:"
Write-Host "1. Add PERSONAL key to personal github account"
Write-Host "2. Add WORK key to work github account"
Write-Host ""
Write-Host "Then test:"
Write-Host ""
Write-Host "ssh -T git@github-personal"
Write-Host "ssh -T git@github-work"
Write-Host ""