#Requires -Version 5.1
<#
.SYNOPSIS
  Configure Claude Code (User / HKCU\Environment): e-lead, wudang, or DeepSeek.

.DESCRIPTION
  e-lead / wudang (minimal, like your shell exports):
    ANTHROPIC_BASE_URL + ANTHROPIC_AUTH_TOKEN (Bearer). Clears ANTHROPIC_API_KEY.
    wudang also sets CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1.

  deepseek (per DeepSeek doc): ANTHROPIC_AUTH_TOKEN + BASE_URL + model tier env vars.

  Clears managed User keys first, strips matching lines from PowerShell profiles, syncs Process, prints verification.

  Run:
      cd D:\source\FKMaster\tools
      .\claude_gateway_env.ps1 -Gateway e-lead
      .\claude_gateway_env.ps1 -Gateway wudang
      .\claude_gateway_env.ps1 -Gateway deepseek

  Optional: -BaseUrl "https://..." if your gateway doc differs from defaults.

  Codex (e.g. Desktop setup_codex.bat) uses OpenAI wire; keys/URLs there are not the same as Claude Code Anthropic wire.

.PARAMETER Gateway
  e-lead | wudang | deepseek

.PARAMETER BaseUrl
  Override ANTHROPIC_BASE_URL (advanced).
#>
param(
    [Parameter(Position = 0)]
    [ValidateSet('e-lead', 'wudang', 'deepseek')]
    [string]$Gateway = 'e-lead',
    [string]$BaseUrl = ''
)

$ErrorActionPreference = 'Stop'

$defaultBase = @{
    'e-lead'   = 'https://e-lead.aiconvert.tech/api'
    'wudang'   = 'https://wudang.aiconvert.tech'
    'deepseek' = 'https://api.deepseek.com/anthropic'
}

$modelOverrideKeys = @(
    'ANTHROPIC_MODEL',
    'ANTHROPIC_DEFAULT_OPUS_MODEL',
    'ANTHROPIC_DEFAULT_SONNET_MODEL',
    'ANTHROPIC_DEFAULT_HAIKU_MODEL',
    'CLAUDE_CODE_SUBAGENT_MODEL',
    'CLAUDE_CODE_EFFORT_LEVEL'
)

$allManagedUserKeys = @(
    'ANTHROPIC_BASE_URL',
    'ANTHROPIC_API_KEY',
    'ANTHROPIC_AUTH_TOKEN',
    'CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC'
) + $modelOverrideKeys

function Mask-Value([string]$s) {
    if ([string]::IsNullOrEmpty($s)) { return '(not set)' }
    if ($s.Length -le 8) { return '***' }
    return $s.Substring(0, 4) + '***' + $s.Substring($s.Length - 4, 4) + " (len=$($s.Length))"
}

function Show-Scopes([string]$name) {
    foreach ($scope in @('User', 'Machine', 'Process')) {
        $v = [Environment]::GetEnvironmentVariable($name, $scope)
        if (-not [string]::IsNullOrEmpty($v)) {
            Write-Host ("  {0}[{1}] = {2}" -f $name, $scope, (Mask-Value $v))
        }
    }
}

function Clear-UserManagedAnthropic {
    foreach ($k in $allManagedUserKeys) {
        $cur = [Environment]::GetEnvironmentVariable($k, 'User')
        if (-not [string]::IsNullOrEmpty($cur)) {
            [Environment]::SetEnvironmentVariable($k, $null, 'User')
            Write-Host "[INFO] Removed User registry: $k"
        }
    }
}

function Test-ProfileLineSetsManagedAnthropic([string]$line) {
    if ([string]::IsNullOrWhiteSpace($line)) { return $false }
    if ($line -match '(?i)^\s*\#') { return $false }
    if ($line -match '(?i)^\s*\$env:ANTHROPIC_') { return $true }
    if ($line -match '(?i)^\s*\$env:CLAUDE_CODE_') { return $true }
    if ($line -match '(?i)SetEnvironmentVariable\s*\(\s*[''"]ANTHROPIC_') { return $true }
    if ($line -match '(?i)SetEnvironmentVariable\s*\(\s*[''"]CLAUDE_CODE_') { return $true }
    if ($line -match '(?i)^\s*setx\s+ANTHROPIC_') { return $true }
    return $false
}

function Get-PowerShellProfileCandidates {
    $candidates = @()
    foreach ($p in @(
            $PROFILE,
            (Join-Path $HOME 'Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1'),
            (Join-Path $HOME 'Documents\WindowsPowerShell\profile.ps1'),
            (Join-Path $HOME 'Documents\PowerShell\Microsoft.PowerShell_profile.ps1'),
            (Join-Path $HOME 'Documents\PowerShell\profile.ps1')
        )) {
        if ([string]::IsNullOrWhiteSpace($p)) { continue }
        try {
            if (Test-Path -LiteralPath $p) {
                $full = (Resolve-Path -LiteralPath $p).Path
                $candidates += $full
            }
        }
        catch { }
    }
    return @($candidates | Select-Object -Unique)
}

function Clear-ManagedAnthropicFromPowerShellProfiles {
    Write-Host '[STEP] Scan PowerShell profiles for ANTHROPIC_/CLAUDE_CODE_ assignments (remove if found)...'
    $paths = @(Get-PowerShellProfileCandidates)
    if ($paths.Count -eq 0) {
        Write-Host '[INFO] No profile files found (nothing to clean).'
        return
    }
    foreach ($path in $paths) {
        $lines = Get-Content -LiteralPath $path -ErrorAction Stop
        $kept = [System.Collections.Generic.List[string]]::new()
        $removed = 0
        foreach ($line in $lines) {
            if (Test-ProfileLineSetsManagedAnthropic $line) {
                $removed++
                Write-Host ("[INFO] Removed line from profile: {0}" -f $path)
                Write-Host ("       >> {0}" -f ($line.TrimEnd() -replace '\S{12,}', '***'))
            }
            else {
                $kept.Add($line) | Out-Null
            }
        }
        if ($removed -gt 0) {
            $bak = $path + '.bak.fkmaster-' + (Get-Date -Format 'yyyyMMddHHmmss')
            Copy-Item -LiteralPath $path -Destination $bak -Force
            Write-Host "[INFO] Backup: $bak"
            $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
            [System.IO.File]::WriteAllLines($path, $kept.ToArray(), $utf8NoBom)
            Write-Host "[INFO] Updated profile ($removed line(s) removed): $path"
        }
        else {
            Write-Host "[INFO] No managed Anthropic/Claude lines in: $path"
        }
    }
}

function Notify-EnvironmentChanged {
    try {
        $typeName = 'FKMasterEnvNotify'
        if (-not ([System.Management.Automation.PSTypeName]$typeName).Type) {
            $code = @'
using System;
using System.Runtime.InteropServices;
public class FKMasterEnvNotify {
  [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
  public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
    uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
}
'@
            Add-Type -TypeDefinition $code -ErrorAction Stop
        }
        $HWND_BROADCAST = [IntPtr]0xffff
        $WM_SETTINGCHANGE = 0x001A
        $SMTO_ABORTIFHUNG = 0x0002
        $r = [UIntPtr]::Zero
        [void][FKMasterEnvNotify]::SendMessageTimeout(
            $HWND_BROADCAST, $WM_SETTINGCHANGE, [UIntPtr]::Zero, 'Environment',
            $SMTO_ABORTIFHUNG, 5000, [ref]$r)
        Write-Host '[INFO] Broadcast WM_SETTINGCHANGE (Environment) for Explorer / other apps.'
    }
    catch {
        Write-Host "[WARN] WM_SETTINGCHANGE failed: $($_.Exception.Message)"
    }
}

function Sync-ExtraUserVarsToProcess {
    param([string[]]$Names)
    foreach ($k in $Names) {
        $u = [Environment]::GetEnvironmentVariable($k, 'User')
        if ([string]::IsNullOrEmpty($u)) {
            [Environment]::SetEnvironmentVariable($k, $null, 'Process')
        }
        else {
            [Environment]::SetEnvironmentVariable($k, $u, 'Process')
        }
    }
}

function Sync-ProcessProfile {
    param(
        [string]$Gw,
        [string]$Base,
        [string]$Token
    )
    [Environment]::SetEnvironmentVariable('ANTHROPIC_BASE_URL', $Base, 'Process')
    if ($Gw -eq 'deepseek') {
        [Environment]::SetEnvironmentVariable('ANTHROPIC_AUTH_TOKEN', $Token, 'Process')
        [Environment]::SetEnvironmentVariable('ANTHROPIC_API_KEY', $null, 'Process')
        Sync-ExtraUserVarsToProcess -Names $modelOverrideKeys
        [Environment]::SetEnvironmentVariable('CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC', $null, 'Process')
    }
    else {
        [Environment]::SetEnvironmentVariable('ANTHROPIC_AUTH_TOKEN', $Token, 'Process')
        [Environment]::SetEnvironmentVariable('ANTHROPIC_API_KEY', $null, 'Process')
        Sync-ExtraUserVarsToProcess -Names $modelOverrideKeys
        Sync-ExtraUserVarsToProcess -Names @('CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC')
    }
    Write-Host '[INFO] Process scope reloaded; use $env:NAME below in this same window.'
}

function Get-EnvDriveValue([string]$name) {
    $item = Get-Item -LiteralPath "Env:\$name" -ErrorAction SilentlyContinue
    if ($null -eq $item) { return $null }
    return $item.Value
}

function Show-Verification {
    Write-Host ''
    Write-Host '======== Verification (User / Process / $env in this window) ========'
    $names = @(
        'ANTHROPIC_BASE_URL',
        'ANTHROPIC_API_KEY',
        'ANTHROPIC_AUTH_TOKEN',
        'CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC'
    ) + $modelOverrideKeys
    foreach ($n in $names) {
        $u = [Environment]::GetEnvironmentVariable($n, 'User')
        $p = [Environment]::GetEnvironmentVariable($n, 'Process')
        $e = Get-EnvDriveValue $n
        Write-Host "  $n"
        if ($n -eq 'ANTHROPIC_BASE_URL' -or $n -eq 'CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC') {
            Write-Host "    [User]    = $(if ($u) { $u } else { '(not set)' })"
            Write-Host "    [Process] = $(if ($p) { $p } else { '(not set)' })"
            Write-Host "    [`$env:]   = $(if ($e) { $e } else { '(not set)' })"
        }
        else {
            Write-Host "    [User]    = $(Mask-Value $u)"
            Write-Host "    [Process] = $(Mask-Value $p)"
            Write-Host "    [`$env:]   = $(Mask-Value $e)"
        }
    }
    Write-Host '======================================================================'
}

$resolvedBase = if (-not [string]::IsNullOrWhiteSpace($BaseUrl)) { $BaseUrl.Trim() } else { $defaultBase[$Gateway] }

Write-Host '========================================'
Write-Host " Claude Code gateway: $Gateway"
Write-Host '========================================'
Write-Host ''
Clear-ManagedAnthropicFromPowerShellProfiles
Write-Host ''
Write-Host 'Current (before):'
Show-Scopes 'ANTHROPIC_BASE_URL'
Show-Scopes 'ANTHROPIC_API_KEY'
Show-Scopes 'ANTHROPIC_AUTH_TOKEN'
Show-Scopes 'CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC'
foreach ($k in $modelOverrideKeys) { Show-Scopes $k }
Write-Host ''
Write-Host "Target ANTHROPIC_BASE_URL (User): $resolvedBase"
Write-Host ''

$keyLabel = switch ($Gateway) {
    'deepseek' { 'Enter your DeepSeek API key (platform.deepseek.com/api_keys)' }
    default    { 'Enter ANTHROPIC_AUTH_TOKEN (Bearer: cr_* or sk-* per gateway)' }
}
$authToken = Read-Host $keyLabel
if ([string]::IsNullOrWhiteSpace($authToken)) {
    Write-Host '[ERROR] Token is empty.'
    exit 1
}
$authToken = $authToken.Trim()

Write-Host ''
Write-Host '[STEP] Remove stale User (HKCU\Environment) keys managed by this script...'
Clear-UserManagedAnthropic

Write-Host ''
Write-Host '[STEP] Apply new User profile...'
[Environment]::SetEnvironmentVariable('ANTHROPIC_BASE_URL', $resolvedBase, 'User')
[Environment]::SetEnvironmentVariable('ANTHROPIC_AUTH_TOKEN', $authToken, 'User')
[Environment]::SetEnvironmentVariable('ANTHROPIC_API_KEY', $null, 'User')

if ($Gateway -eq 'deepseek') {
    [Environment]::SetEnvironmentVariable('ANTHROPIC_MODEL', 'deepseek-v4-pro', 'User')
    [Environment]::SetEnvironmentVariable('ANTHROPIC_DEFAULT_OPUS_MODEL', 'deepseek-v4-pro', 'User')
    [Environment]::SetEnvironmentVariable('ANTHROPIC_DEFAULT_SONNET_MODEL', 'deepseek-v4-pro', 'User')
    [Environment]::SetEnvironmentVariable('ANTHROPIC_DEFAULT_HAIKU_MODEL', 'deepseek-v4-flash', 'User')
    [Environment]::SetEnvironmentVariable('CLAUDE_CODE_SUBAGENT_MODEL', 'deepseek-v4-flash', 'User')
    [Environment]::SetEnvironmentVariable('CLAUDE_CODE_EFFORT_LEVEL', 'max', 'User')
    Write-Host '[INFO] deepseek: BASE_URL + AUTH_TOKEN + DeepSeek model envs (no ANTHROPIC_API_KEY).'
}
elseif ($Gateway -eq 'wudang') {
    [Environment]::SetEnvironmentVariable('CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC', '1', 'User')
    Write-Host '[INFO] wudang: BASE_URL + AUTH_TOKEN + CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1 (no ANTHROPIC_API_KEY).'
}
else {
    Write-Host '[INFO] e-lead: BASE_URL + AUTH_TOKEN only (no ANTHROPIC_API_KEY).'
}

Notify-EnvironmentChanged

Write-Host ''
Write-Host '[STEP] Reload Process env in this session ($env: matches intent)...'
Sync-ProcessProfile -Gw $Gateway -Base $resolvedBase -Token $authToken

$mApi = [Environment]::GetEnvironmentVariable('ANTHROPIC_API_KEY', 'Machine')
$mAuth = [Environment]::GetEnvironmentVariable('ANTHROPIC_AUTH_TOKEN', 'Machine')
$mBase = [Environment]::GetEnvironmentVariable('ANTHROPIC_BASE_URL', 'Machine')
if ($mApi -or $mAuth -or $mBase) {
    Write-Host ''
    Write-Host '[WARN] Machine (HKLM) still has Anthropic vars - script cannot remove HKLM without Admin.'
    if ($mBase) { Write-Host ('  ANTHROPIC_BASE_URL[Machine] = ' + (Mask-Value $mBase)) }
    if ($mApi) { Write-Host ('  ANTHROPIC_API_KEY[Machine]   = ' + (Mask-Value $mApi)) }
    if ($mAuth) { Write-Host ('  ANTHROPIC_AUTH_TOKEN[Machine]= ' + (Mask-Value $mAuth)) }
    Write-Host '  Remove in System Properties > Environment Variables if they override User.'
}

Show-Verification

Write-Host ''
Write-Host 'Quick echo:'
Write-Host "  ANTHROPIC_BASE_URL=$([Environment]::GetEnvironmentVariable('ANTHROPIC_BASE_URL','Process'))"
Write-Host "  ANTHROPIC_API_KEY   = $(Mask-Value ([Environment]::GetEnvironmentVariable('ANTHROPIC_API_KEY','Process')))"
Write-Host "  ANTHROPIC_AUTH_TOKEN= $(Mask-Value ([Environment]::GetEnvironmentVariable('ANTHROPIC_AUTH_TOKEN','Process')))"
$dnt = [Environment]::GetEnvironmentVariable('CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC', 'Process')
Write-Host "  CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=$(if ($dnt) { $dnt } else { '(not set)' })"

Write-Host ''
if ($Gateway -eq 'deepseek') {
    Write-Host 'DeepSeek: https://api-docs.deepseek.com/quick_start/agent_integrations/claude_code'
}
else {
    Write-Host 'e-lead / wudang: Bearer ANTHROPIC_AUTH_TOKEN + ANTHROPIC_BASE_URL (same idea as export in shell).'
    Write-Host 'setx OPENAI_API_KEY does not apply to Claude Code Anthropic requests; safe to ignore or remove.'
}
Write-Host ''
Write-Host '========================================'
Write-Host ' DONE'
Write-Host '========================================'
Write-Host 'Logoff or reboot if a new Explorer shell still shows old values.'
Write-Host 'In this window: claude'
Write-Host ''
Write-Host 'Refs: https://code.claude.com/docs/en/env-vars'
Write-Host ''
