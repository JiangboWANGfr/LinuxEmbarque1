#include "fpga.h"
#include <QDebug>

#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>


#include <sys/mman.h>
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"



//#include "hps_0.h"

// QSyS dependent address
#define FPGA_LED_PIO_BASE   0x03000
#define FPGA_SW_PIO_BASE    0x04000
#define FPGA_KEY_PIO_BASE   0x05000
#define FPGA_HEX_BASE       0x33000


#define FPGA_VIP_CVI_BASE   0x36000
#define FPGA_VIP_MIX_BASE   0x32000
#define FPGA_IR_RX_BASE     0x34000

#define FPGA_VIP_SCL_BASE           0x37000  // scaler for compositive-in
#define FPGA_VIP_SCL_CAMERA_BASE    0x39000 // scaler for camera
#define FPGA_VIP_SWITCH_BASE        0x3A000 // switch for camera or compositive-in

//#define FPGA_ADC_SPI_BASE   0x40040



// ///////////////////////////////////////
// memory map
//#define ALT_STM_OFST        0xfc000000  定义在 hps.h 

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

// ///////////////////////////////////////////////////
// SPI Micro
#define ALTERA_AVALON_SPI_RXDATA_REG                  0
#define ALTERA_AVALON_SPI_TXDATA_REG                  1
#define ALTERA_AVALON_SPI_STATUS_REG                  2
#define ALTERA_AVALON_SPI_CONTROL_REG                 3
#define ALTERA_AVALON_SPI_SLAVE_SEL_REG               5
#define IORD(base, index)                             (*( ((uint32_t *)base)+index))
#define IOWR(base, index, data)                       (*(((uint32_t *)base)+index) = data)
#define IORD_ALTERA_AVALON_SPI_RXDATA(base)           IORD(base, ALTERA_AVALON_SPI_RXDATA_REG)
#define IORD_ALTERA_AVALON_SPI_STATUS(base)           IORD(base, ALTERA_AVALON_SPI_STATUS_REG)
#define IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, data)  IOWR(base, ALTERA_AVALON_SPI_SLAVE_SEL_REG, data)
#define IOWR_ALTERA_AVALON_SPI_CONTROL(base, data)    IOWR(base, ALTERA_AVALON_SPI_CONTROL_REG, data)
#define IOWR_ALTERA_AVALON_SPI_TXDATA(base, data)     IOWR(base, ALTERA_AVALON_SPI_TXDATA_REG, data)

#define ALTERA_AVALON_SPI_STATUS_ROE_MSK              (0x8)
#define ALTERA_AVALON_SPI_STATUS_ROE_OFST             (3)
#define ALTERA_AVALON_SPI_STATUS_TOE_MSK              (0x10)
#define ALTERA_AVALON_SPI_STATUS_TOE_OFST             (4)
#define ALTERA_AVALON_SPI_STATUS_TMT_MSK              (0x20)
#define ALTERA_AVALON_SPI_STATUS_TMT_OFST             (5)
#define ALTERA_AVALON_SPI_STATUS_TRDY_MSK             (0x40)
#define ALTERA_AVALON_SPI_STATUS_TRDY_OFST            (6)
#define ALTERA_AVALON_SPI_STATUS_RRDY_MSK             (0x80)
#define ALTERA_AVALON_SPI_STATUS_RRDY_OFST            (7)
#define ALTERA_AVALON_SPI_STATUS_E_MSK                (0x100)
#define ALTERA_AVALON_SPI_STATUS_E_OFST               (8)

// end
// ///////////////////////////////////////////////////

FPGA::FPGA() :
    m_bIsVideoEnabled(false)
{
    //
    m_mpu9250.initialize();
    m_adc9300.Set_PowerSwitch(true); //power on

    //
    m_bInitSuccess = Init();
    if (!m_bInitSuccess)
        qDebug() << "FPGA init failed!!!\r\n";
}

FPGA::~FPGA()
{
    close(m_file_mem);
}



bool FPGA::Init()
{
    bool bSuccess = false;

    m_file_mem = open( "/dev/mem", ( O_RDWR | O_SYNC ) );
    if (m_file_mem != -1){
        void *virtual_base;
        virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, m_file_mem, HW_REGS_BASE );
        if (virtual_base == MAP_FAILED){
        }else{
            m_led_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_LED_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_key_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_KEY_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_sw_base = (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_SW_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_hex_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_HEX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_vip_cvi_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_VIP_CVI_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_vip_mix_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_VIP_MIX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_switch_video_in_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_VIP_SWITCH_BASE ) & ( unsigned long)( HW_REGS_MASK ) );


            m_ir_rx_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_IR_RX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            //m_adc_spi_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_ADC_SPI_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            bSuccess = true;


        }
        close(m_file_mem);
    }


    return bSuccess;
}


bool FPGA::LedSet(int mask){
    if (!m_bInitSuccess)
        return false;

    //qDebug() << "FPGA:LedSet\r\n";
    *(uint32_t *)m_led_base = mask;
    return true;
}


bool FPGA::HexSet(int index, int value){
    if (!m_bInitSuccess)
        return false;

    uint8_t szMask[] = {
        63, 6, 91, 79, 102, 109, 125, 7,
        127, 111, 119, 124, 57, 94, 121, 113
    };

    if (value < 0)
        value = 0;
    else if (value > 15)
        value = 15;

    //qDebug() << "index=" << index << "value=" << value << "\r\n";

    *((uint32_t *)m_hex_base+index) = szMask[value];
    return true;
}

bool FPGA::KeyRead(uint32_t *mask){
    if (!m_bInitSuccess)
        return false;

    *mask = *(uint32_t *)m_key_base;
    return true;

}

bool FPGA::SwitchRead(uint32_t *mask){
    if (!m_bInitSuccess)
        return false;

    *mask = *(uint32_t *)m_sw_base;
    return true;

}

bool FPGA::IrDataRead(uint32_t *scan_code){
    if (!m_bInitSuccess)
        return false;

    *scan_code = *(uint32_t *)m_ir_rx_base;

    return true;
}


bool FPGA::IrIsDataReady(void){

    if (!m_bInitSuccess)
        return false;

    uint32_t status;
    status = *(((uint32_t *)m_ir_rx_base)+1);

    if (status)
        return true;
    return false;

}



bool FPGA::VideoEnable(bool bEnable){
    if (!m_bInitSuccess)
        return false;

    const int nLayerIndex = 1;

    //qDebug() << "Video-In" << (bEnable?"Yes":"No") << "\r\n";

    IOWR(m_vip_cvi_base, 0x00, bEnable?0x01:0x00);
#if 1
    // mixer II
    IOWR(m_vip_mix_base, 8+nLayerIndex*5+2, bEnable?0x01:0x00);
#else
    // mixer
    IOWR(m_vip_mix_base, nLayerIndex*3+1, bEnable?0x01:0x00);
#endif

    if (bEnable)
        VideoMove(0,0);

    m_bIsVideoEnabled = bEnable;

    return true;
}

bool FPGA::VideoMove(int x, int y){
    if (!m_bInitSuccess)
        return false;

    const int nLayerIndex = 1;
#if 1
    // mixer II
    IOWR(m_vip_mix_base, 8+nLayerIndex*5+0, x);
    IOWR(m_vip_mix_base, 8+nLayerIndex*5+1, y);
#else
    // mixer
    IOWR(m_vip_mix_base, nLayerIndex*3-1, x);
    IOWR(m_vip_mix_base, nLayerIndex*3+0, y);
#endif
    return true;
}

// SWICH II
bool FPGA::SwitchVideoIn(VIDEO_IN_ID VideoInID){
    int nInIndex = 0;
    switch(VideoInID){
        case VIDEO_IN_CAMERA: nInIndex = 1 /* 3'b001=din_0 */; break;
        case VIDEO_IN_COMPOSITIVE: nInIndex = 2 /* 3'b010=din_1 */; break;
    }

    IOWR(m_switch_video_in_base, 0x04 /*dout0 output Control*/, nInIndex); // output control
    IOWR(m_switch_video_in_base, 0x03, 0x01); // output switch. set 1 to take effect
    return true;
}

bool FPGA::CameraFocus(int nFocusPos /* 0 ~ 1023 */){
    bool bSuccess;
    bSuccess = m_camera_vm149c.WriteFocus(nFocusPos);
    return bSuccess;
}


bool FPGA::IsVideoEnabled(){
    return m_bIsVideoEnabled;
}



bool FPGA::getMotion9(float *ax, float *ay, float *az, float *gx, float *gy, float *gz, float *mx, float *my, float *mz){
    m_mpu9250.getMotion9(ax, ay, az, gx, gy, gz, mx, my, mz);
    return true;
}

bool FPGA::getLight(uint16_t *light0, uint16_t *light1){
    bool bSuccess;

    bSuccess = m_adc9300.Get_ADCData0(light0);
    if (bSuccess)
        bSuccess = m_adc9300.Get_ADCData1(light1);

    return bSuccess;
}
