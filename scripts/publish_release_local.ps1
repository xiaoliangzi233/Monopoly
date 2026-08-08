[CmdletBinding()]
param(
    [string]$Version,
    [switch]$SkipTests,
    [switch]$DryRun,
    [switch]$Yes
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$githubRepository = 'xiaoliangzi233/Monopoly'
$giteeRepository = 'small-quantum/Monopoly'
$publicKey = '075c6b007b90986ada19a50ab52cf3bfbe4d16aef72673be0d2928f00f746e0d'
$githubManifestUrl = 'https://raw.githubusercontent.com/xiaoliangzi233/Monopoly/update-manifests/updates/github/stable.json'
$giteeManifestUrl = 'https://gitee.com/small-quantum/Monopoly/raw/update-manifests/updates/gitee/stable.json'

function Invoke-External {
    param([string]$File, [string[]]$Arguments, [string]$FailureMessage)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$FailureMessage (exit $LASTEXITCODE)" }
}

Set-Location $root
foreach ($tool in @('git', 'gh', 'cmake', 'ctest', 'cpack', 'python')) {
    if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Required tool is missing: $tool" }
}
Invoke-External 'gh' @('auth', 'status') 'GitHub CLI is not authenticated'
Invoke-External 'python' @('-c', 'import nacl, requests') `
    'Python release dependencies are missing; run: python -m pip install pynacl requests'
if ([string]::IsNullOrWhiteSpace($env:UPDATE_SIGNING_KEY)) {
    throw 'UPDATE_SIGNING_KEY is not configured locally. It must be the original Base64 Ed25519 private key.'
}
if ([string]::IsNullOrWhiteSpace($env:GITEE_TOKEN)) {
    throw 'GITEE_TOKEN is not configured locally.'
}
Invoke-External 'python' @('scripts/validate_signing_key.py', '--public-key-hex', $publicKey) `
    'The local signing key cannot sign updates for existing launchers'

$branch = (git branch --show-current).Trim()
if ($branch -ne 'main') { throw "Run releases from main, current branch is '$branch'" }
$dirty = @(git status --porcelain | Where-Object { $_ -notmatch '^\?\? \.VSCodeCounter/' })
if ($dirty.Count -gt 0) { throw "Working tree is not clean:`n$($dirty -join "`n")" }
$current = (Get-Content -Raw -Encoding UTF8 (Join-Path $root 'VERSION')).Trim()
if (!$Version) { $Version = $current }
if ($Version -notmatch '^\d+\.\d+\.\d+$' -or $Version -ne $current) {
    throw "Version must match VERSION ($current)"
}
$tag = "v$Version"
$existingTag = (git tag --list $tag).Trim()
if ($existingTag) {
    $tagCommit = (git rev-list -n 1 $tag).Trim()
    $headCommit = (git rev-parse HEAD).Trim()
    if ($tagCommit -ne $headCommit) { throw "Tag $tag points to $tagCommit instead of current HEAD $headCommit" }
    Write-Host "Retrying the existing tag $tag from the same tested commit."
}

Write-Host "Local release : $tag"
Write-Host 'Publishing does not use GitHub Actions.'
if (!$Yes) {
    $answer = Read-Host 'Build, sign and publish to GitHub/Gitee? [y/N]'
    if ($answer -notin @('y', 'Y', 'yes', 'YES')) { throw 'Release cancelled' }
}

$qtPrefix = if ($env:NEON_QT_PREFIX) { $env:NEON_QT_PREFIX } else { 'C:\msys64\mingw64' }
$env:PATH = (Join-Path $qtPrefix 'bin') + ';C:\Program Files (x86)\NSIS;' + $env:PATH
$build = Join-Path $root 'build\local-release'
$dist = Join-Path $root 'build\local-release-dist'
Invoke-External 'cmake' @('-S', $root, '-B', $build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_PREFIX_PATH=$qtPrefix", "-DNEON_UPDATE_PUBLIC_KEY_HEX=$publicKey",
    "-DNEON_GITHUB_MANIFEST_URL=$githubManifestUrl", "-DNEON_GITEE_MANIFEST_URL=$giteeManifestUrl") `
    'CMake configure failed'
Invoke-External 'cmake' @('--build', $build, '-j', '4') 'Build failed'
if (!$SkipTests) {
    $env:NEON_SIMULATION_GAMES = '10000'
    Invoke-External 'ctest' @('--test-dir', $build, '--output-on-failure') 'Tests failed'
}
Invoke-External 'cpack' @('--config', (Join-Path $build 'CPackConfig.cmake'), '-G', 'NSIS', '-B', $dist) `
    'NSIS packaging failed'
$asset = Get-ChildItem $dist -Filter "ShengshiBaiye-$Version-win64.exe" | Select-Object -First 1
if (!$asset) { throw 'Expected installer was not generated' }

if ($DryRun) {
    Write-Host "Dry run passed: $($asset.FullName)"
    exit 0
}

if (!$existingTag) {
    Invoke-External 'git' @('tag', '-a', $tag, '-m', "Shengshi Baiye $Version") 'Tag creation failed'
}
Invoke-External 'git' @('push', 'origin', 'main') 'GitHub main push failed'
Invoke-External 'git' @('push', 'gitee', 'main') 'Gitee main push failed'
Invoke-External 'git' @('push', 'origin', $tag) 'GitHub tag push failed'
Invoke-External 'git' @('push', 'gitee', $tag) 'Gitee tag push failed'
& gh release view $tag --repo $githubRepository *> $null
if ($LASTEXITCODE -eq 0) {
    Invoke-External 'gh' @('release', 'upload', $tag, $asset.FullName, '--repo', $githubRepository, '--clobber') `
        'GitHub Release asset upload failed'
} else {
    Invoke-External 'gh' @('release', 'create', $tag, $asset.FullName, '--repo', $githubRepository,
        '--title', "盛世百业 $Version", '--generate-notes', '--verify-tag') 'GitHub Release publishing failed'
}
$releaseJson = gh release view $tag --repo $githubRepository --json assets
if ($LASTEXITCODE -ne 0) { throw 'Unable to inspect the GitHub Release' }
$release = $releaseJson | ConvertFrom-Json
$githubUrl = (($release.assets | Where-Object { $_.name -eq $asset.Name } | Select-Object -First 1).url)
if (!$githubUrl) { throw 'GitHub did not return the installer download URL' }
$giteeUrl = (& python scripts/publish_gitee.py --repository $giteeRepository --tag $tag `
    --asset $asset.FullName --replace-existing).Trim()
if ($LASTEXITCODE -ne 0 -or !$giteeUrl) { throw 'Gitee Release publishing failed' }

$generated = Join-Path $root 'build\local-release-manifests'
Invoke-External 'python' @('scripts/sign_manifest.py', '--channel', 'stable', '--version', $Version,
    '--url', $githubUrl, '--file', $asset.FullName, '--output', (Join-Path $generated 'github\stable.json')) `
    'GitHub manifest signing failed'
Invoke-External 'python' @('scripts/sign_manifest.py', '--channel', 'stable', '--version', $Version,
    '--url', $giteeUrl, '--file', $asset.FullName, '--output', (Join-Path $generated 'gitee\stable.json')) `
    'Gitee manifest signing failed'

Invoke-External 'git' @('fetch', 'origin', 'update-manifests') 'Unable to fetch manifest branch'
Invoke-External 'git' @('branch', '-f', 'update-manifests', 'origin/update-manifests') 'Unable to update local manifest branch'
$worktree = Join-Path ([System.IO.Path]::GetTempPath()) "shengshi-manifests-$PID"
if (Test-Path -LiteralPath $worktree) { throw "Temporary worktree already exists: $worktree" }
try {
    Invoke-External 'git' @('worktree', 'add', $worktree, 'update-manifests') 'Unable to create manifest worktree'
    New-Item -ItemType Directory -Force -Path (Join-Path $worktree 'updates\github'), (Join-Path $worktree 'updates\gitee') | Out-Null
    Copy-Item -Force (Join-Path $generated 'github\stable.json') (Join-Path $worktree 'updates\github\stable.json')
    Copy-Item -Force (Join-Path $generated 'gitee\stable.json') (Join-Path $worktree 'updates\gitee\stable.json')
    Invoke-External 'git' @('-C', $worktree, 'add', 'updates') 'Unable to stage manifests'
    & git -C $worktree diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        Invoke-External 'git' @('-C', $worktree, '-c', 'user.name=shengshi-release-bot',
            '-c', 'user.email=release-bot@users.noreply.github.com', 'commit', '-m', "Update stable manifest to $Version") `
            'Manifest commit failed'
    } else {
        Write-Host 'Signed manifests already contain this release.'
    }
    Invoke-External 'git' @('-C', $worktree, 'push', 'origin', 'update-manifests') 'GitHub manifest push failed'
    Invoke-External 'git' @('-C', $worktree, 'push', 'gitee', 'update-manifests') 'Gitee manifest push failed'
} finally {
    if (Test-Path -LiteralPath $worktree) { git worktree remove --force $worktree *> $null }
}

Write-Host "Published $tag without GitHub Actions."
Write-Host "GitHub: https://github.com/$githubRepository/releases/tag/$tag"
Write-Host "Gitee : https://gitee.com/$giteeRepository/releases/tag/$tag"
