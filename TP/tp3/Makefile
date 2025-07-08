KERNEL_SOURCE ?=/home/ensea/linux-socfpga
obj-m += gpio_leds.o 
CFLAGS_MODULE=-fno-pic
all :
	make -C $(KERNEL_SOURCE) M=$(PWD) modules
	arm-linux-gnueabihf-gcc usr.c -o usr
clean :
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
	rm -rf usr
cp:
	cp *.ko ~/src/modules
	cp usr ~/src/modules
install :
	make −C $(KERNEL_SOURCE) M=$(PWD) modules install

