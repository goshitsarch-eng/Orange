param([Parameter(Mandatory=$true)][string]$GStreamerRoot)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '../..')
$version = (Select-String '^version = "([^"]+)"' Cargo.toml).Matches[0].Groups[1].Value
$stage = Join-Path (Get-Location) "target/packages/orange-$version-windows-x86_64"
if (Test-Path $stage) {Remove-Item -Recurse -Force $stage}
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item -Recurse 'desktop/build/windows/x64/runner/Release/*' $stage
if (!(Test-Path "$stage/orange_bridge.dll")) {throw 'Flutter bundle is missing the Rust bridge'}
Copy-Item "$GStreamerRoot/bin/*.dll" $stage
New-Item -ItemType Directory -Force "$stage/lib", "$stage/libexec" | Out-Null
Copy-Item -Recurse "$GStreamerRoot/lib/gstreamer-1.0" "$stage/lib/"
Copy-Item -Recurse "$GStreamerRoot/libexec/gstreamer-1.0" "$stage/libexec/"
# GIO/TLS modules can be required by network source plugins.
if (Test-Path "$GStreamerRoot/lib/gio") {Copy-Item -Recurse "$GStreamerRoot/lib/gio" "$stage/lib/"}
Copy-Item COPYING,README.md,PLATFORM_SUPPORT.md,BUILDING.md,MIGRATION_AUDIT.md,ARCHITECTURE.md,CONTRIBUTING.md,CHANGELOG.md $stage
Copy-Item -Recurse docs "$stage/docs"
if (Test-Path "$GStreamerRoot/share/licenses") {Copy-Item -Recurse "$GStreamerRoot/share/licenses" "$stage/licenses"}
$archive = "$stage.zip"
Compress-Archive -Path "$stage/*" -DestinationPath $archive -Force
$nsis = Join-Path ${env:ProgramFiles(x86)} 'NSIS/makensis.exe'
& $nsis "/DVERSION=$version" "/DSTAGE=$stage" "/DOUTPUT=$stage-setup.exe" packaging/windows/installer.nsi
if ($LASTEXITCODE -ne 0) {throw 'NSIS build failed'}
foreach ($file in @($archive,"$stage-setup.exe")) {
    $hash=(Get-FileHash -Algorithm SHA256 $file).Hash.ToLower()
    [System.IO.File]::WriteAllText("$file.sha256", "$hash  $(Split-Path -Leaf $file)`n", [System.Text.Encoding]::ASCII)
}
