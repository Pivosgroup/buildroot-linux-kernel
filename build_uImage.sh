#/bin/sh
make -j4
make modules -j4
mkdir -p ramdisk/root/boot
cp -vf drivers/amlogic/ump/ump.ko ../out/target/product/stvm3/root/boot/
#cp -vf drivers/amlogic/ump/ump.ko ramdisk/root_release/boot/
mkdir -p ramdisk/root_release/boot
cp -vf drivers/amlogic/mali/mali.ko ../out/target/product/stvm3/root/boot/
#cp -vf drivers/amlogic/mali/mali.ko ramdisk/root_release/boot/
make uImage -j4
cp -rf arch/arm/boot/uImage ../out/target/product/stvm3/
