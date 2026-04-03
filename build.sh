@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo COSMOS OS Build Script (Windows Native)
echo ==========================================

:: Pfade anpassen
set "LLVM_PATH=C:\Program Files\LLVM\bin"
set "QEMU_PATH=C:\Program Files\qemu"
set "MINGW_PATH=C:\mingw64\bin"

set "PATH=%LLVM_PATH%;%QEMU_PATH%;%MINGW_PATH%;%PATH%"

:: Verzeichnisse erstellen
if not exist build mkdir build
if not exist isoroot mkdir isoroot\boot\limine

echo [1/6] Checking tools...

where clang >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Clang nicht gefunden. Bitte installieren:
    echo https://github.com/llvm/llvm-project/releases
    pause
    exit /b 1
)

echo [2/6] Compiling kernel...

:: Kernel mit Clang kompilieren (cross-compile für x86_64-elf)
clang -target x86_64-elf -O2 -pipe -Wall -Wextra -ffreestanding ^
    -fno-stack-protector -fno-stack-check -fPIE -m64 -march=x86-64 ^
    -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel ^
    -fno-exceptions -fno-rtti -std=c++17 ^
    -Iinclude -c src\kernel.cpp -o build\kernel.o

if %errorlevel% neq 0 (
    echo COMPILATION FAILED!
    pause
    exit /b 1
)

echo [3/6] Linking...

:: Linken mit LLVM lld
lld-link -flavor ld -m elf_x86_64 -static -pie --no-dynamic-linker ^
    -z text -z max-page-size=0x1000 -T linker.ld ^
    build\kernel.o -o build\cosmos.elf

if %errorlevel% neq 0 (
    echo LINKING FAILED!
    pause
    exit /b 1
)

echo [4/6] Downloading Limine...

if not exist limine (
    git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
)

echo [5/6] Creating ISO structure...

copy build\cosmos.elf isoroot\boot\ >nul
copy limine.conf isoroot\boot\limine\ >nul
copy limine\limine-bios.sys isoroot\boot\limine\ >nul
copy limine\limine-bios-cd.bin isoroot\boot\limine\ >nul
copy limine\limine-uefi-cd.bin isoroot\boot\limine\ >nul

echo [6/6] Building ISO...

:: xorriso muss installiert sein (von MSYS2 oder Cygwin)
where xorriso >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: xorriso nicht gefunden. Versuche alternativen Build...
    goto :alt_build
)

xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin ^
    -no-emul-boot -boot-load-size 4 -boot-info-table ^
    --efi-boot boot/limine/limine-uefi-cd.bin ^
    -efi-boot-part --efi-boot-image --protective-msdos-label ^
    isoroot -o cosmos_os.iso

goto :success

:alt_build
echo Versuche alternativen ISO Build mit PowerShell...
powershell -Command "$bootFile='isoroot/boot/limine/limine-bios-cd.bin'; $iso='cosmos_os.iso'; & { write-host 'Bitte installiere xorriso von MSYS2: pacman -S xorriso' }"

:success
echo.
echo ==========================================
echo BUILD COMPLETE: cosmos_os.iso
echo ==========================================

if exist cosmos_os.iso (
    echo.
    echo Test mit QEMU:
    echo   qemu-system-x86_64 -cdrom cosmos_os.iso -m 512M -vga std
)

pause