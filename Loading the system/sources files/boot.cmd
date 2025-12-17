echo "Hello from boot.scr"

echo "Загружаю zImage..."
fatload mmc 0:1 0x62000000 zImage
echo "zImage успешно загружен!"

echo "Загружаю initramfs.img..."
fatload mmc 0:1 0x63000000 initramfs.img
echo "initramfs.img успешно загружен!"

echo "Загружаю dtb..."
fatload mmc 0:1 0x64000000 vexpress-v2p-ca9.dtb
echo "dtb успешно загружен!"

echo "Настраиваю env var..."
setenv bootargs console=ttyAMA0 rootwait rw
echo "env var успешно настроены"

echo "Поехали!"
bootz 0x62000000 0x63000000 0x64000000
