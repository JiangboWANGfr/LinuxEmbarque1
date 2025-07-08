KERNEL_SOURCE ?=/home/ensea/linux-socfpga
obj-m += afficheur.o 
CFLAGS_MODULE=-fno-pic
all :
	make -C $(KERNEL_SOURCE) M=$(PWD) modules
	arm-linux-gnueabihf-gcc usr.c -o usr
	arm-linux-gnueabihf-gcc fpga_usr.c -o fpga_usr
clean :
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
	rm -rf usr
	rm -rf fpga_usr
cp:
	cp *.ko ~/src/modules
	cp usr ~/src/modules
install :
	make −C $(KERNEL_SOURCE) M=$(PWD) modules install

