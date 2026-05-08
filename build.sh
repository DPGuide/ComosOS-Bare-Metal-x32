#!/bin/bash
set -e

echo "=== AUTOMATISCHES BACKUP ==="
BACKUP_DIR="backups/cosmos_$(date +%Y-%m-%d_%H-%M-%S)"
mkdir -p "$BACKUP_DIR"
# Kopiert NUR die .cpp und .h Dateien ins Backup!
cp *.cpp *.h "$BACKUP_DIR/" 2>/dev/null || true
echo "-> Quellcodes sicher kopiert nach: $BACKUP_DIR"
echo "============================"

echo "0. Alten Müll aufräumen..."
rm -f *.o *.elf cosmos.iso
rm -rf isodir
mkdir -p isodir/boot/grub

echo "1. Erstelle 64-Bit Linker-Skript..."
cat > linker64.ld << 'EOF'
ENTRY(os2_start)
SECTIONS
{
    . = 0x1000000;
    .text : ALIGN(4096) {
        *(.text.entry)
        *(.text .text.*)
    }
    .rodata : ALIGN(4096) { *(.rodata .rodata.*) }
    .data : ALIGN(4096) { *(.data .data.*) }
    .bss : ALIGN(4096) { *(COMMON) *(.bss .bss.*) }
}
EOF

# ==========================================
# OS 1: 32-BIT BOOTLOADER / DISK MANAGER
# ==========================================
echo "2. Kompiliere OS1 (32-Bit)..."
as --32 boot.s -o boot.o
g++ -m32 -O2 -c kernel.cpp -o kernel_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c pci.cpp -o pci_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c net.cpp -o net_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c cosmos_bytes.cpp -o cosmos_bytes_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive
g++ -m32 -O2 -c cosmos_fs.cpp -o cosmos_fs_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c cosmos_tba.cpp -o cosmos_tba_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive
g++ -m32 -O2 -c cosmos_ahci.cpp -o cosmos_ahci_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c cosmos_cfs.cpp -o cosmos_cfs_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c cosmos_usb.cpp -o cosmos_usb_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m32 -O2 -c cosmos_partition.cpp -o cosmos_partition_32.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast

ld -m elf_i386 -T linker.ld -static -o isodir/boot/kernel.bin boot.o kernel_32.o pci_32.o net_32.o cosmos_bytes_32.o cosmos_fs_32.o cosmos_tba_32.o cosmos_ahci_32.o cosmos_cfs_32.o cosmos_usb_32.o cosmos_partition_32.o

# ==========================================
# OS 2: 64-BIT PAYLOAD (KERNEL V2)
# ==========================================
echo "3. Kompiliere OS2 (64-Bit)..."
as --64 os2_entry.s -o os2_entry.o
g++ -m64 -mno-red-zone -O2 -c kernel_main.cpp -o kernel_main_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c kernel.cpp -o kernel_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c pci.cpp -o pci_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c net.cpp -o net_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c cosmos_bytes.cpp -o cosmos_bytes_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive
g++ -m64 -mno-red-zone -O2 -c cosmos_fs.cpp -o cosmos_fs_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c cosmos_tba.cpp -o cosmos_tba_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive
g++ -m64 -mno-red-zone -O2 -c cosmos_ahci.cpp -o cosmos_ahci_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c cosmos_cfs.cpp -o cosmos_cfs_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -mno-red-zone -O2 -c cosmos_usb.cpp -o cosmos_usb_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast
g++ -m64 -c arcade.cpp -o arcade.o -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -mno-red-zone
g++ -m64 -mno-red-zone -O2 -c cosmos_partition.cpp -o cosmos_partition_64.o -ffreestanding -fno-exceptions -fno-rtti -fpermissive -Wno-int-to-pointer-cast

echo "4. Linke OS2 (Die flache KERNEL.BIN)..."
ld -m elf_x86_64 -T linker64.ld --allow-multiple-definition os2_entry.o kernel_main_64.o kernel_64.o pci_64.o net_64.o cosmos_bytes_64.o cosmos_fs_64.o cosmos_tba_64.o cosmos_ahci_64.o cosmos_cfs_64.o cosmos_usb_64.o arcade.o cosmos_partition_64.o -o kernel_main.elf

objcopy -O binary kernel_main.elf isodir/KERNEL.BIN

# ==========================================
# NEU: EXTERNE APP KOMPILIEREN & BRENNEN
# ==========================================
echo "-> Kompiliere externe App (app.cpp)..."
g++ -m64 -mno-red-zone -O2 -c app.cpp -o app.o -ffreestanding -fno-exceptions -fno-rtti
ld -m elf_x86_64 --oformat binary -Ttext 0x0 app.o -o app.bin

echo "-> Erstelle SATA-Festplatte und brenne App..."
# Falls hdd.img nicht existiert, erstellen wir eine leere 10 MB Platte
if [ ! -f hdd.img ]; then
    dd if=/dev/zero of=hdd.img bs=1M count=10 status=none
fi
# App.bin auf Sektor 10000 brennen (ohne den Rest der Platte zu löschen)
dd if=app.bin of=hdd.img bs=512 seek=10000 conv=notrunc status=none
# ==========================================

echo "5. ISO erstellen..."
cat > isodir/boot/grub/grub.cfg << EOF
# ... restlicher Code ...
set timeout=0
set default=0
menuentry "Cosmos OS" {
    set gfxpayload=800x600x32
    multiboot /boot/kernel.bin
    module /KERNEL.BIN
    boot
}
EOF
grub-mkrescue -o cosmos.iso isodir
echo "ISO ERFOLGREICH GEBAUT!"