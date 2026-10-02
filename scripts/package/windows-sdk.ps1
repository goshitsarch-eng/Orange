$ErrorActionPreference='Stop'
$version='1.26.5'
$root='C:\gstreamer\1.0\msvc_x86_64'
foreach ($name in @('gstreamer-1.0-msvc-x86_64','gstreamer-1.0-devel-msvc-x86_64')) {
    $file="$name-$version.msi"
    $url="https://gstreamer.freedesktop.org/data/pkg/windows/$version/msvc/$file"
    Invoke-WebRequest $url -OutFile "$env:TEMP/$file"
    Invoke-WebRequest "$url.sha256sum" -OutFile "$env:TEMP/$file.sha256sum"
    $expected=(Get-Content "$env:TEMP/$file.sha256sum" -Raw).Trim().Split(' ')[0].ToLower()
    if ($expected -notmatch '^[0-9a-f]{64}$' -or (Get-FileHash "$env:TEMP/$file" -Algorithm SHA256).Hash.ToLower() -ne $expected) {throw "GStreamer checksum mismatch: $file"}
    $process=Start-Process msiexec.exe -ArgumentList @('/i',"`"$env:TEMP/$file`"",'/quiet','/norestart','ADDLOCAL=ALL') -Wait -PassThru
    if ($process.ExitCode -notin @(0,3010)) {throw "GStreamer installation failed: $($process.ExitCode)"}
}
if (!(Test-Path "$root/bin/gst-inspect-1.0.exe")) {throw 'GStreamer SDK installation was not found'}
"GSTREAMER_1_0_ROOT_MSVC_X86_64=$root" >> $env:GITHUB_ENV
"PKG_CONFIG_PATH=$root/lib/pkgconfig" >> $env:GITHUB_ENV
"$root/bin" >> $env:GITHUB_PATH
