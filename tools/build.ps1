param([switch]$Upload, [string]$Port = "COM15")
$ErrorActionPreference = "Stop"
$abyssProject = Split-Path -Parent $PSScriptRoot
$abyssCli = Join-Path $env:LOCALAPPDATA "Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
if (!(Test-Path -LiteralPath $abyssCli)) { $abyssCli = (Get-Command arduino-cli -ErrorAction Stop).Source }
# The Windows linker needs an ASCII output path in this environment.
$abyssBuild = Join-Path $env:TEMP "letter-tab5-build"
# Serial is the USB-Serial/JTAG on the USB-C socket, so CDC On Boot has to be enabled;
# with it disabled Arduinos Serial goes to the UART0 pins, which nothing is wired to.
# ChipVariant=prev3 is the esp32p4_es library set, which is what a P4 below revision
# v3.00 wants; `esptool chip-id` on this board reports v1.3.
$abyssFqbn = "esp32:esp32:m5stack_tab5:ChipVariant=prev3,PSRAM=enabled,PartitionScheme=custom,CPUFreq=360,FlashMode=qio,USBMode=hwcdc,CDCOnBoot=cdc,UploadMode=default"
# The shipped arduino-cli.yaml carries placeholder paths; arduino-cli.local.yaml, when it
# is there, carries this machine's and is not in the repository.
$abyssConfig = Join-Path $abyssProject "arduino-cli.local.yaml"
if (!(Test-Path -LiteralPath $abyssConfig)) { $abyssConfig = Join-Path $abyssProject "arduino-cli.yaml" }
& $abyssCli --config-file $abyssConfig compile --fqbn $abyssFqbn --build-path $abyssBuild --warnings all $abyssProject
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
if ($Upload) {
    # arduino-cli's own upload gives up part way through an image this size and leaves the
    # board unbootable, so the flash is written directly at a calmer rate.
    $abyssEsptool = Get-ChildItem -Path (Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\tools\esptool_py") -Filter esptool.exe -Recurse |
        Sort-Object FullName | Select-Object -Last 1
    if (!$abyssEsptool) { throw "esptool.exe not found" }
    $abyssBoot = Get-ChildItem -Path (Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\hardware\esp32") -Filter boot_app0.bin -Recurse |
        Sort-Object FullName | Select-Object -Last 1
    if (!$abyssBoot) { throw "boot_app0.bin not found" }
    $abyssName = Split-Path -Leaf $abyssProject
    # The P4 keeps its bootloader at 0x2000, not 0x0.
    & $abyssEsptool.FullName --chip esp32p4 --port $Port --baud 921600 --before default-reset --after hard-reset `
        write-flash -z --flash-mode qio --flash-freq 80m --flash-size 16MB `
        0x2000 (Join-Path $abyssBuild "$abyssName.ino.bootloader.bin") `
        0x8000 (Join-Path $abyssBuild "$abyssName.ino.partitions.bin") `
        0xe000 $abyssBoot.FullName `
        0x10000 (Join-Path $abyssBuild "$abyssName.ino.bin")
    if ($LASTEXITCODE -ne 0) { throw "Upload failed ($LASTEXITCODE)" }
}
