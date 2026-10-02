param(
    [string]$Message,
    [string[]]$Paths = @(),
    [ValidateRange(1, 120)][int]$TimeoutMinutes = 30
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repo = 'baseredge/ScreenCapture'
$remote = 'private'

function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command failed (exit $LASTEXITCODE)." }
}

Push-Location $PSScriptRoot
try {
    foreach ($command in @('git', 'gh')) {
        if (-not (Get-Command $command -ErrorAction SilentlyContinue)) { throw "$command is required." }
    }
    $branch = Invoke-Checked git @('branch', '--show-current')
    if ($branch -ne 'main') { throw 'Publish requires the main branch.' }
    $url = Invoke-Checked git @('remote', 'get-url', '--push', $remote)
    if ($url -notmatch '^(git@github\.com:|https://github\.com/)baseredge/ScreenCapture(\.git)?$') {
        throw "Unexpected push destination: $url"
    }
    $null = Invoke-Checked gh @('repo', 'view', $repo, '--json', 'nameWithOwner')
    Invoke-Checked git @('fetch', $remote, 'main')
    & git merge-base --is-ancestor "$remote/main" HEAD
    if ($LASTEXITCODE -ne 0) { throw 'Remote main has changes not in local main. Integrate them before publishing.' }

    if ($Paths.Count -gt 0) {
        if ([string]::IsNullOrWhiteSpace($Message)) { throw '-Message is required when staging files.' }
        Invoke-Checked git (@('add', '--') + $Paths)
    }
    else {
        $changes = Invoke-Checked git @('status', '--porcelain')
        if ($changes) {
            if ([string]::IsNullOrWhiteSpace($Message)) { throw '-Message is required to commit working tree changes.' }
            Invoke-Checked git @('add', '--all')
        }
    }
    & git diff --cached --quiet
    if ($LASTEXITCODE -eq 1) {
        if ([string]::IsNullOrWhiteSpace($Message)) { throw '-Message is required to commit staged changes.' }
        Invoke-Checked git @('commit', '-m', $Message)
    }
    elseif ($LASTEXITCODE -ne 0) { throw 'Could not inspect staged changes.' }

    $sha = Invoke-Checked git @('rev-parse', 'HEAD')
    Invoke-Checked git @('push', $remote, 'main')
    Write-Host "Waiting for release workflow for $sha..."
    $deadline = (Get-Date).AddMinutes($TimeoutMinutes)
    $run = $null
    while ((Get-Date) -lt $deadline) {
        $runs = @( (Invoke-Checked gh @('run', 'list', '--repo', $repo, '--workflow', 'release.yml', '--branch', 'main', '--commit', $sha, '--limit', '5', '--json', 'databaseId,status,conclusion,url')) | ConvertFrom-Json )
        if ($runs.Count -gt 0) {
            $run = $runs[0]
            if ($run.status -eq 'completed') { break }
            Write-Host "Workflow $($run.databaseId): $($run.status)"
        }
        Start-Sleep -Seconds 10
    }
    if (-not $run -or $run.status -ne 'completed') { throw 'Release timed out. The cloud workflow may still be running.' }
    if ($run.conclusion -ne 'success') { throw "Release workflow $($run.conclusion): $($run.url)" }
    $release = (Invoke-Checked gh @('release', 'view', 'latest', '--repo', $repo, '--json', 'body,url,assets,targetCommitish')) | ConvertFrom-Json
    if ($release.body -notmatch [regex]::Escape("Commit: $sha") -or $release.targetCommitish -ne $sha) {
        throw 'Latest release does not match this commit; another push may have replaced it.'
    }
    $sourceSha = Invoke-Checked gh @('api', "repos/$repo/git/ref/tags/latest", '--jq', '.object.sha')
    if ($sourceSha -ne $sha) { throw 'Release source tag does not match this commit.' }
    foreach ($name in @('ScreenCapture-windows-x64.zip', 'ScreenCapture-windows-x64.sha256')) {
        if (-not @($release.assets | Where-Object { $_.name -eq $name -and $_.size -gt 0 }).Count) {
            throw "Release asset missing or empty: $name"
        }
    }
    Write-Host "Published commit $sha"
    Write-Host $release.url
}
finally { Pop-Location }
