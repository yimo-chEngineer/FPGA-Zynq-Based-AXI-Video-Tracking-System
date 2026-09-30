#include "xparameters.h"
#include "xiicps.h"
#include <sleep.h>
#include <xil_printf.h>
#include <xil_types.h>
#include <xstatus.h>
#include <xaxivdma.h>
#include <xvtc.h>

#define XIICPS_BASEADDR     XPAR_XIICPS_0_BASEADDR
#define XIICPS_CLK          100000
#define ov7670_addr         0x21

#define XVDMA_BASEADDR      XPAR_XAXIVDMA_0_BASEADDR
#define XVDMA_FRAMEADDR1    0x1f000000
#define XVDMA_FRAMEADDR2    0x1f100000
#define VDMA_FRAMECOUNT     3

#define XVTC_BASEADDR       XPAR_XVTC_0_BASEADDR

#define HORIZONTAL_SIZE     1280
#define VERTICAL_SIZE       480

XIicPs Iic;
XAxiVdma Vdma;
XVtc Vtc;

XIicPs_Config *i2cConfigPtr;
XAxiVdma_Config *vdmaConfigPtr;
XVtc_Config *vtcConfigPtr;



//****************************** FUNCTION DECLARATIONS ******************************//

int iic_configuration_setup(u32 base_addr);
int iic_initialize(XIicPs *iic_instance, XIicPs_Config *i2cConfigPtr, u32 base_addr, int clk_frequency);
int iic_write(XIicPs *iic_instance, u8 reg_addr, u8 data);
int iic_read(XIicPs *iic_instance, u8 *MsgPtr, u8 reg_addr);

int vdma_configuration_setup(u32 base_addr);
int vdma_initialize(XAxiVdma *vdmainstance, XAxiVdma_Config *vdmaConfigPtr, UINTPTR base_addr);
int vdma_setframecount(XAxiVdma *vdma_instance, u8 frame_buffer_size, u16 Direction);
int vdma_writesetup (XAxiVdma_DmaSetup *write_config);
int vdma_readsetup (XAxiVdma_DmaSetup *read_config);

int vtc_configuration_setup (u32 base_addr);
int vtc_initialize(XVtc *InstancePtr, XVtc_Config *CfgPtr, UINTPTR EffectiveAddr);

//****************************** MAIN ******************************//

int main(void) {
    //Configures ov7670
    if(iic_configuration_setup(XIICPS_BASEADDR) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    //Initializes I2C
    if (iic_initialize(&Iic, i2cConfigPtr, XIICPS_BASEADDR, XIICPS_CLK) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    //Reset SCCB to default
    if (iic_write(&Iic, 0x12, 0x80) == XST_FAILURE) { 
        xil_printf("Reset 1 failure\n");
        return XST_FAILURE;
    } usleep(100000);

    //Product ID and version for verification
    u8 pid;
    u8 ver;
    if (iic_read(&Iic, &pid, 0x0A) == XST_FAILURE || iic_read(&Iic, &ver, 0x0B) == XST_FAILURE) { //Reads for Product ID and Product Version
        xil_printf("Could not read identification\n");
        return XST_FAILURE;
    }
    if (pid != 0x76 || ver != 0x73) {
        xil_printf("Incorrect ov7670 identification\n");
        return XST_FAILURE;
    }
    
    //Reset SCCB to default
    if (iic_write(&Iic, 0x12, 0x80) == XST_FAILURE) {
        xil_printf("Reset 1 failure\n");
        return XST_FAILURE;
    } usleep(100000);
    
    //Configurate ov7670 settings
    u8 configuration_values[][2] = {{0x11,0x01},{0x6b,0x4a},{0x3a,0x04},{0x12,0x04},{0x40,0xd0},{0x8c,0x00},{0x17,0x16},{0x18,0x04},{0x32, 0x24},{0x19, 0x02},{0x1A, 0x7A},{0x03, 0x0A},{0x4f,0xb3},{0x50,0xb3},{0x51,0x00},{0x52,0x3d},{0x53,0xa7},{0x54,0xe4},{0x58,0x9e},{0x3d,0xc0}};
    for (unsigned int i = 0; i < sizeof(configuration_values) / sizeof(configuration_values[0]); i++) {
        if(iic_write(&Iic, configuration_values[i][0], configuration_values[i][1]) == XST_FAILURE) {
            xil_printf("Register %02x calibration failure\n", configuration_values[i][0]);        
            return XST_FAILURE;
        }
    }
    xil_printf("OV7670 camera configuration successful!\n");


    //Set up vdma for configuration
    if (vdma_configuration_setup(XVDMA_BASEADDR) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    xil_printf("Before VDMA initialization\n");
    //Initialize vdma
    if(vdma_initialize(&Vdma, vdmaConfigPtr, XVDMA_BASEADDR) == XST_FAILURE) {
        return XST_FAILURE;
    }
    xil_printf("After VDMA initialization\n");

    //Set vdma frame count for write
    if (vdma_setframecount(&Vdma, VDMA_FRAMECOUNT, XAXIVDMA_WRITE) == XST_FAILURE) {
        return XST_FAILURE;
    }

    //Set vdma frame count for write
    if (vdma_setframecount(&Vdma, VDMA_FRAMECOUNT, XAXIVDMA_READ) == XST_FAILURE) {
        return XST_FAILURE;
    }

    //Configurate vdma read settings
    XAxiVdma_DmaSetup vdma_write_setup = {0};
    if (vdma_writesetup(&vdma_write_setup) == XST_FAILURE) {
        return XST_FAILURE;
    }

    //Configurate vdma write settings
    XAxiVdma_DmaSetup vdma_read_setup = {0};
    if (vdma_readsetup(&vdma_read_setup) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    //Set up VTC Configuration
    if (vtc_configuration_setup (XVTC_BASEADDR) == XST_FAILURE) {
        return XST_FAILURE;
    }

    //Initialize VTC
    if (vtc_initialize(&Vtc, vtcConfigPtr, XVTC_BASEADDR) == XST_FAILURE) {
        return XST_FAILURE;
    }

    //Configurate vtc settings
    XVtc_Timing vtc_timing = {0};
    if (vtc_configurate(&vtc_timing));
}

//****************************** I2C CONFIGURATION SETUP ******************************//

int iic_configuration_setup(u32 base_addr) {
    i2cConfigPtr = XIicPs_LookupConfig(base_addr);
    if (i2cConfigPtr == NULL) {
        xil_printf("I2C configuration Address not found\n");
        return XST_FAILURE;
    } xil_printf("I2C configuration success\n");
    return XST_SUCCESS;
}



//****************************** I2C INITIALIZATION ******************************//

int iic_initialize(XIicPs *iic_instance, XIicPs_Config *i2cConfigPtr, u32 base_addr, int clk_frequency) {
    if ((XIicPs_CfgInitialize(iic_instance, i2cConfigPtr, base_addr) == XST_SUCCESS) && (XIicPs_SetSClk(iic_instance, clk_frequency) == XST_SUCCESS)) {
        xil_printf("I2C initialization success\n");
        return XST_SUCCESS;
    }
    xil_printf("I2C initialization failure\n");
    return XST_FAILURE;
}



//****************************** I2C WRITE ******************************//

int iic_write(XIicPs *iic_instance, u8 reg_addr, u8 data) {
    u8 data_array[2];
    data_array[0] = reg_addr;
    data_array[1] = data;
    if (XIicPs_MasterSendPolled(iic_instance, &data_array[0], 2, ov7670_addr) == XST_SUCCESS) {
        while (XIicPs_BusIsBusy(iic_instance)); // Wait for bus to idle
        xil_printf("I2C write success\n");
        return XST_SUCCESS;
    }
    xil_printf("I2C write failure\n");
    return XST_FAILURE;
}



//****************************** I2C READ ******************************//

int iic_read(XIicPs *iic_instance, u8 *MsgPtr, u8 reg_addr) {
    if (XIicPs_MasterSendPolled(iic_instance, &reg_addr, 1, ov7670_addr) == XST_SUCCESS) {
        while (XIicPs_BusIsBusy(iic_instance)); // Wait for bus to idle
        if (XIicPs_MasterRecvPolled(iic_instance, MsgPtr, 1, ov7670_addr) == XST_SUCCESS) {
            while (XIicPs_BusIsBusy(iic_instance)); // Wait for bus to idle
            xil_printf("I2C read success\n");
            return XST_SUCCESS;
        }
    }
    xil_printf("I2C read failure\n");
    return XST_FAILURE;
}



//****************************** VDMA CONFIGURATION SETUP ******************************//

int vdma_configuration_setup(u32 base_addr) {
    vdmaConfigPtr = XAxiVdma_LookupConfig(base_addr);
    if (vdmaConfigPtr == NULL) {
        xil_printf("Configuration Address not found\n");
        return XST_FAILURE;
    } 
    xil_printf("VDMA set up success\n");
    return XST_SUCCESS;
}



//****************************** VDMA INITIALIZATION ******************************//

int vdma_initialize(XAxiVdma *vdmainstance, XAxiVdma_Config *vdmaConfigPtr, UINTPTR base_addr) {
    xil_printf("Testing VDMA access...\r\n");

    u32 test = Xil_In32(XVDMA_BASEADDR);

    xil_printf("VDMA CR = 0x%08lx\r\n", test);
    if (XAxiVdma_CfgInitialize(vdmainstance, vdmaConfigPtr, base_addr) == XST_SUCCESS) {
        xil_printf("VMDA initialization success\n");
        return XST_SUCCESS;
    }
    xil_printf("VDMA initialization failure\n");
    return XST_FAILURE;
}



//****************************** VDMA SET FRAME COUNT ******************************//

int vdma_setframecount (XAxiVdma *vdma_instance, u8 frame_buffer_size, u16 Direction) {
    if (XAxiVdma_SetFrmStore(vdma_instance, frame_buffer_size, Direction) == XST_SUCCESS) {
        xil_printf("VDMA set frame count success\n");
        return XST_SUCCESS;
    }
    xil_printf("VDMA set frame count failure\n");
    return XST_FAILURE;
}



//****************************** VDMA WRITE SET UP & CONFIGURATION ******************************//

int vdma_writesetup (XAxiVdma_DmaSetup *write_config) {
    write_config->VertSizeInput = VERTICAL_SIZE;
    write_config->HoriSizeInput = HORIZONTAL_SIZE;
    write_config->Stride = HORIZONTAL_SIZE;

    write_config->FrameDelay = 0;
    write_config->EnableCircularBuf = 1;
    write_config->EnableSync = 1;
    write_config->PointNum = 0;
    write_config->GenLockRepeat = 0;
    write_config->EnableFrameCounter = 0;
    write_config->FixedFrameStoreAddr = 0;
     
    for (int i = 0; i < VDMA_FRAMECOUNT; i++) {
        write_config->FrameStoreStartAddr[i] = XVDMA_FRAMEADDR1 + (i * 0x100000);
    }
    if (XAxiVdma_DmaConfig(&Vdma, XAXIVDMA_WRITE, write_config) == XST_SUCCESS) {
        if (XAxiVdma_DmaSetBufferAddr(&Vdma, XAXIVDMA_WRITE, write_config->FrameStoreStartAddr) == XST_SUCCESS) {
            xil_printf("VDMA Write Configuration Success\n");
            if (XAxiVdma_DmaStart(&Vdma, XAXIVDMA_WRITE) == XST_SUCCESS) {
                xil_printf("VDMA Write Start Success\n");
                return XST_SUCCESS;
            } else {
                xil_printf("VDMA Write Start Failure\n");
                return XST_FAILURE;
            }
        } else {
            xil_printf("VDMA Write Set Buffer Failure\n");
            return XST_FAILURE;
        }
    } 
    else {
        xil_printf("VDMA Write Configuration Failure\n");
        return XST_FAILURE;
    }
}



//****************************** VDMA READ SET UP & CONFIGURATION ******************************//

int vdma_readsetup (XAxiVdma_DmaSetup *read_config) {
    read_config->VertSizeInput = VERTICAL_SIZE;
    read_config->HoriSizeInput = HORIZONTAL_SIZE;
    read_config->Stride = HORIZONTAL_SIZE;

    read_config->FrameDelay = 0;
    read_config->EnableCircularBuf = 1;
    read_config->EnableSync = 1;
    read_config->PointNum = 0;
    read_config->GenLockRepeat = 1;
    read_config->EnableFrameCounter = 0;
    read_config->FixedFrameStoreAddr = 0;
    for (int i = 0; i < VDMA_FRAMECOUNT; i++) {
        read_config->FrameStoreStartAddr[i] = XVDMA_FRAMEADDR1 + (i * 0x100000);
    }
    if (XAxiVdma_DmaConfig(&Vdma, XAXIVDMA_READ, read_config) == XST_SUCCESS) {
        if (XAxiVdma_DmaSetBufferAddr(&Vdma, XAXIVDMA_READ, read_config->FrameStoreStartAddr) == XST_SUCCESS) {
            xil_printf("VDMA Read Configuration Success\n");
            if (XAxiVdma_DmaStart(&Vdma, XAXIVDMA_READ) == XST_SUCCESS) {
                xil_printf("VDMA Read Start Success\n");
                return XST_SUCCESS;
            } else {
                xil_printf("VDMA Read Start Failure\n");
                return XST_FAILURE;
            }
        } else {
            xil_printf("VDMA Read Set Buffer Failure\n");
            return XST_FAILURE;
        }
    }
    else {
        xil_printf("VDMA Read Configuration Failure\n");
        return XST_FAILURE;
    }
}



//****************************** VTC CONFIGURATION SET UP ******************************//

int vtc_configuration_setup (u32 base_addr) {
    vtcConfigPtr = XVtc_LookupConfig(base_addr);
    if (vtcConfigPtr == NULL) {
        xil_printf("VTC configuration address not found\n");
        return XST_FAILURE;
    }
    xil_printf("VTC configuration success\n");
    return XST_SUCCESS;
}



//****************************** VTC INITIALIZATION ******************************//

int vtc_initialize(XVtc *InstancePtr, XVtc_Config *CfgPtr, UINTPTR EffectiveAddr) {
    if (XVtc_CfgInitialize(InstancePtr, CfgPtr, EffectiveAddr) == XST_SUCCESS) {
        xil_printf("Vtc Initialization Success\n");
        return XST_SUCCESS;
    }
    xil_printf("Vtc Initialization Failure\n");
    return XST_FAILURE;
}



//****************************** VTC SETTINGS CONFIGURATION ******************************//

int vtc_configurate(XVtc_Timing *timing) {
    timing->HActiveVideo = 640;
    timing->HFrontPorch = 80;
    XVtc_SetGeneratorTiming(&Vtc, timing);
}
