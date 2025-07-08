#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include "stdio.h"
#include <sys/mman.h>



// ///////////////////////////////////////
// memory map
#define HW_REGS_BASE 			( 0xfc000000 )
#define HW_REGS_SPAN 			( 0x04000000 )
#define HW_REGS_MASK 			( HW_REGS_SPAN - 1 )
#define FPGA_HEX_BASE       	0x33000
#define  ALT_LWFPGASLVS_OFST 	0xff200000

uint8_t *m_hex_base;	

static int FPGAInit()
{
 int m_file_mem;
 
    m_file_mem = open( "/dev/mem", ( O_RDWR | O_SYNC ) );
    if (m_file_mem != -1){
        void *virtual_base;
        virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, m_file_mem, HW_REGS_BASE );
        if (virtual_base == MAP_FAILED){
        }else{
            m_hex_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_HEX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
			//0xfc000000+(0xff200000+0x33000)&(0x04000000-1)
        }
        close(m_file_mem);
    }

    return 0;
}


static int  HexSet(int index, int value){

    uint8_t szMask[] = {
        63, 6, 91, 79, 102, 109, 125, 7,
        127, 111, 119, 124, 57, 94, 121, 113, 64, 0
    };

    if (value < 0)
        value = 0;
    else if (value > 17)
        value = 17;

    //qDebug() << "index=" << index << "value=" << value << "\r\n";

    *((uint32_t *)m_hex_base+index) = szMask[value];
    return 0;
}

int main()
{

	FPGAInit();
	HexSet(0,1);
	HexSet(1,2);
	HexSet(2,3);
	HexSet(3,4);
	HexSet(4,5);
	HexSet(5,6);
	return 0 ;
}
