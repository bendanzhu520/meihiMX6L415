cd u-boot-imx6-2015.04/
make distclean
make mx6qsabresd_config
#make mx6solosabresd_config
make -j8
mv u-boot.imx /home/harry/TftpRoot/uboot-mh-imx6-6q.imx
