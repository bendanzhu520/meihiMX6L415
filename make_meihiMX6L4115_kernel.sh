cd  kernel-imx6-4.1.15/
make distclean
make imx_v7_defconfig
make -j8 zImage
make imx6q-sabresd.dtb
make imx6dl-sabresd.dtb

# Example:
# mv arch/arm/boot/zImage /home/harry/TftpRoot/zImage-mhimx6
# mv arch/arm/boot/dts/imx6q-sabresd.dtb  /home/harry/TftpRoot/mhimx6-6q.dtb
# mv arch/arm/boot/dts/mh-imx6dl-sabresd.dtb  /home/harry/TftpRoot/mhimx6-6s.dtb