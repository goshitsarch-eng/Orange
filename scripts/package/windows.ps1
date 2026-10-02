param([Parameter(Mandatory=$true)][string]$GStreamerRoot)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '../..')
$version = (Select-String '^version = "([^"]+)"' Cargo.toml).Matches[0].Groups[1].Value
$stage = Join-Path (Get-Location) "target/packages/orange-$version-windows-x86_64"
if (Test-Path $stage) {Remove-Item -Recurse -Force $stage}
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item target/release/orange.exe $stage
Copy-Item "$GStreamerRoot/bin/*.dll" $stage
New-Item -ItemType Directory -Force "$stage/lib", "$stage/libexec" | Out-Null
Copy-Item -Recurse "$GStreamerRoot/lib/gstreamer-1.0" "$stage/lib/"
Copy-Item -Recurse "$GStreamerRoot/libexec/gstreamer-1.0" "$stage/libexec/"
# GIO/TLS modules can be required by network source plugins.
if (Test-Path "$GStreamerRoot/lib/gio") {Copy-Item -Recurse "$GStreamerRoot/lib/gio" "$stage/lib/"}
Copy-Item COPYING,README.md,PLATFORM_SUPPORT.md,BUILDING.md,MIGRATION_AUDIT.md,ARCHITECTURE.md,CONTRIBUTING.md,CHANGELOG.md $stage
Copy-Item -Recurse docs "$stage/docs"
if (Test-Path "$GStreamerRoot/share/licenses") {Copy-Item -Recurse "$GStreamerRoot/share/licenses" "$stage/licenses"}
Invoke-WebRequest 'https://go.microsoft.com/fwlink/p/?LinkId=2124703' -OutFile "$stage/MicrosoftEdgeWebview2Setup.exe"
$signature = Get-AuthenticodeSignature "$stage/MicrosoftEdgeWebview2Setup.exe"
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft') {throw 'WebView2 installer signature validation failed'}
$archive = "$stage.zip"
Compress-Archive -Path "$stage/*" -DestinationPath $archive -Force
$nsis = Join-Path ${env:ProgramFiles(x86)} 'NSIS/makensis.exe'
& $nsis "/DVERSION=$version" "/DSTAGE=$stage" "/DOUTPUT=$stage-setup.exe" dist/dioxus/windows/installer.nsi
if ($LASTEXITCODE -ne 0) {throw 'NSIS build failed'}
foreach ($file in @($archive,"$stage-setup.exe")) {
    $hash=(Get-FileHash -Algorithm SHA256 $file).Hash.ToLower()
    "$hash  $(Split-Path -Leaf $file)" | Set-Content -Encoding ascii "$file.sha256"
}
