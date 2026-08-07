[CmdletBinding()]
param(
    [string]$Version,
    [string]$RetryTag,
    [switch]$SkipTests,
    [switch]$NoWait,
    [switch]$DryRun,
    [switch]$Yes
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$repository = 'xiaoliangzi233/Monopoly'
$versionFile = Join-Path $root 'VERSION'

function Invoke-External {
    param([string]$File, [string[]]$Arguments, [string]$FailureMessage)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$FailureMessage (exit $LASTEXITCODE)" }
}

function Get-ReleaseRuns {
    $json = gh run list --repo $repository --workflow release.yml --limit 30 `
        --json databaseId,headSha,status,conclusion,url,createdAt
    if ($LASTEXITCODE -ne 0) { throw 'Unable to query GitHub Actions runs' }
    if (!$json) { return @() }
    $parsed = $json | ConvertFrom-Json
    if ($null -eq $parsed) { return @() }
    return @($parsed)
}

function Wait-ForReleaseRun {
    param([string]$Commit, [long[]]$KnownRunIds)
    $deadline = (Get-Date).AddMinutes(3)
    $run = $null
    while ((Get-Date) -lt $deadline -and $null -eq $run) {
        Start-Sleep -Seconds 3
        $run = Get-ReleaseRuns | Where-Object {
            $KnownRunIds -notcontains [long]$_.databaseId -and
                (!$Commit -or $_.headSha -eq $Commit)
        } | Select-Object -First 1
    }
    if ($null -eq $run) { throw 'Release workflow did not appear within three minutes' }
    Write-Host "Release workflow: $($run.url)"
    if (!$NoWait) {
        & gh run watch $run.databaseId --repo $repository --exit-status
        if ($LASTEXITCODE -ne 0) {
            throw "Release workflow failed: $($run.url)"
        }
    }
}

Set-Location $root
foreach ($tool in @('git', 'gh')) {
    if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Required tool is missing: $tool" }
}
Invoke-External -File 'gh' -Arguments @('auth', 'status') -FailureMessage 'GitHub CLI is not authenticated'

[string]$branch = git branch --show-current
$branch = $branch.Trim()
if ($branch -ne 'main') { throw "Run releases from main, current branch is '$branch'" }
$dirty = @(git status --porcelain)
if ($dirty.Count -gt 0) { throw "Working tree is not clean:`n$($dirty -join "`n")" }
foreach ($remote in @('origin', 'gitee')) {
    git remote get-url $remote *> $null
    if ($LASTEXITCODE -ne 0) { throw "Missing git remote: $remote" }
}

if ($RetryTag) {
    if ($RetryTag -notmatch '^v[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$') {
        throw 'RetryTag must look like v0.1.0 or v0.2.0-beta.1'
    }
    $known = @(Get-ReleaseRuns | ForEach-Object { [long]$_.databaseId })
    [string]$retryTagMatch = git tag --list $RetryTag
    if (!$retryTagMatch.Trim()) { throw "Tag does not exist locally: $RetryTag" }
    Invoke-External -File 'gh' -Arguments @('workflow', 'run', 'release.yml', '--repo', $repository,
        '--ref', 'main', '-f', "tag=$RetryTag") -FailureMessage 'Unable to dispatch release workflow'
    Wait-ForReleaseRun -Commit '' -KnownRunIds $known
    exit 0
}

$current = (Get-Content -Raw -Encoding UTF8 $versionFile).Trim()
if ($current -notmatch '^(\d+)\.(\d+)\.(\d+)$') { throw "Invalid VERSION file: $current" }
if (!$Version) {
    $Version = '{0}.{1}.{2}' -f $Matches[1], $Matches[2], ([int]$Matches[3] + 1)
}
if ($Version -notmatch '^(\d+)\.(\d+)\.(\d+)$') {
    throw 'Version must look like 0.1.1'
}
$numericVersion = '{0}.{1}.{2}' -f $Matches[1], $Matches[2], $Matches[3]
$tag = "v$Version"
[string]$existingTag = git tag --list $tag
if ($existingTag.Trim()) { throw "Tag already exists: $tag; use -RetryTag $tag instead" }

Write-Host "Current version : $current"
Write-Host "Release version : $Version"
Write-Host "Release tag     : $tag"
if (!$Yes) {
    $answer = Read-Host 'Publish this version to GitHub and Gitee? [y/N]'
    if ($answer -notin @('y', 'Y', 'yes', 'YES')) { throw 'Release cancelled' }
}

if (!$SkipTests) {
    foreach ($tool in @('cmake', 'ctest')) {
        if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Required tool is missing: $tool" }
    }
    $qtPrefix = if ($env:NEON_QT_PREFIX) { $env:NEON_QT_PREFIX } else { 'C:\msys64\mingw64' }
    if (!(Test-Path (Join-Path $qtPrefix 'bin'))) {
        throw "Qt prefix not found: $qtPrefix (set NEON_QT_PREFIX to override)"
    }
    $env:PATH = (Join-Path $qtPrefix 'bin') + ';' + $env:PATH
    $checkBuild = Join-Path $root 'build\publish-check'
    Invoke-External -File 'cmake' -Arguments @('-S', $root, '-B', $checkBuild, '-G', 'Ninja',
        "-DCMAKE_PREFIX_PATH=$qtPrefix", '-DCMAKE_BUILD_TYPE=Release') -FailureMessage 'CMake configure failed'
    Invoke-External -File 'cmake' -Arguments @('--build', $checkBuild, '-j', '4') -FailureMessage 'Build failed'
    $env:NEON_SIMULATION_GAMES = '10000'
    Invoke-External -File 'ctest' -Arguments @('--test-dir', $checkBuild, '--output-on-failure') `
        -FailureMessage 'Tests failed'
}

if ($DryRun) {
    Write-Host "Dry run passed. No files, commits, tags, releases, or remotes were changed."
    exit 0
}

$knownRuns = @(Get-ReleaseRuns | ForEach-Object { [long]$_.databaseId })
$committed = $false
try {
    [System.IO.File]::WriteAllText($versionFile, "$numericVersion`n", [System.Text.UTF8Encoding]::new($false))
    Invoke-External -File 'git' -Arguments @('add', 'VERSION') -FailureMessage 'Unable to stage VERSION'
    Invoke-External -File 'git' -Arguments @('commit', '-m', "Release $tag") -FailureMessage 'Version commit failed'
    $committed = $true
    Invoke-External -File 'git' -Arguments @('push', 'gitee', 'main') -FailureMessage 'Gitee main push failed'
    Invoke-External -File 'git' -Arguments @('push', 'origin', 'main') -FailureMessage 'GitHub main push failed'
    Invoke-External -File 'git' -Arguments @('tag', '-a', $tag, '-m', "Neon Tycoon $Version") `
        -FailureMessage 'Tag creation failed'
    Invoke-External -File 'git' -Arguments @('push', 'gitee', $tag) -FailureMessage 'Gitee tag push failed'
    Invoke-External -File 'git' -Arguments @('push', 'origin', $tag) -FailureMessage 'GitHub tag push failed'
} catch {
    if (!$committed) {
        [System.IO.File]::WriteAllText($versionFile, "$current`n", [System.Text.UTF8Encoding]::new($false))
        git reset VERSION *> $null
    }
    throw
}

[string]$commit = git rev-list -n 1 $tag
$commit = $commit.Trim()
Wait-ForReleaseRun -Commit $commit -KnownRunIds $knownRuns
Write-Host "Published $tag"
Write-Host "GitHub: https://github.com/$repository/releases/tag/$tag"
Write-Host "Gitee : https://gitee.com/small-quantum/Monopoly/releases/tag/$tag"
