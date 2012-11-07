#/bin/sh
export PATH=/opt/arm-2010q1/bin:$PATH
#
make meson_stv_mbx_m3_initrd_defconfig
make -j4
make modules -j4
rm -rf ramdisk/root/boot
mkdir -p ramdisk/root/boot
cp -vf drivers/amlogic/ump/ump.ko ramdisk/root/boot/
cp -vf drivers/amlogic/mali/mali.ko ramdisk/root/boot/
make uImage -j4
cp -rf arch/arm/boot/uImage ../out/target/product/stvm3/
