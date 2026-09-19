#include "xparameters.h"
#include "xiicps.h"
#include <sleep.h>
#include <xil_printf.h>
#include <xil_types.h>
#include <xstatus.h>

#define XIICPS_BASEADDR XPAR_XIICPS_0_BASEADDR
#define XIICPS_CLK      100000
#define ov7670_addr     0x21

XIicPs Iic;
XIicPs_Config *ConfigPtr;


//****************************** FUNCTION DECLARATIONS ******************************//

int iic_initialize(XIicPs *iic_instance, XIicPs_Config *ConfigPtr, u32 base_addr, int clk_frequency);
int iic_write(XIicPs *iic_instance, u8 reg_addr, u8 data);
int iic_read(XIicPs *iic_instance, u8 *MsgPtr, u8 reg_addr);



//****************************** MAIN ******************************//

int main(void) {
    //Configures device
    ConfigPtr = XIicPs_LookupConfig(XIICPS_BASEADDR);
    if (ConfigPtr == NULL) {
        xil_printf("Configuration Address not found\n");
        return XST_FAILURE;
    }
    //Initializes I2C
    if (iic_initialize(&Iic, ConfigPtr, XIICPS_BASEADDR, XIICPS_CLK) == XST_FAILURE) {
        return XST_FAILURE;
    }
    
    //Product ID and version for verification
    u8 pid;
    u8 ver;
    if (iic_write(&Iic, 0x12, 0x80) == XST_FAILURE) { //Reset SCCB to default
        xil_printf("Reset 1 failure");
        return XST_FAILURE;
    }
    usleep(100000);
    if (iic_read(&Iic, &pid, 0x0A) == XST_FAILURE || iic_read(&Iic, &ver, 0x0B) == XST_FAILURE) { //Reads for Product ID and Product Version
        xil_printf("Could not read identification");
        return XST_FAILURE;
    }
    //ID verification
    if (pid != 0x76 || ver != 0x73) {
        xil_printf("Incorrect ov7670 identification\n");
        return XST_FAILURE;
    }
    
    if (iic_write(&Iic, 0x12, 0x80) == XST_FAILURE) { //Reset SCCB to default
        xil_printf("Reset 1 failure");
        return XST_FAILURE;
    }
    usleep(100000);
    u8 configuration_values[10][2] = {{0x11,0x00},{0x12,0x04},{0x40,0xd0},{0x8c,0x00},{0x17,0x16},{0x18,0x04},{0x32, 0x24},{0x19, 0x02},{0x1A, 0x7A},{0x03, 0x0A}};
    for (int i = 0; i < 10; i++) {
        if(iic_write(&Iic, configuration_values[i][0], configuration_values[i][1]) == XST_FAILURE) {
            xil_printf("Register %02x calibration failure\n", configuration_values[i][0]);        
            return XST_FAILURE;
        }
    }
    xil_printf("OV7670 camera calibration successful!");
}



//****************************** I2C INITIALIZATION ******************************//

int iic_initialize(XIicPs *iic_instance, XIicPs_Config *ConfigPtr, u32 base_addr, int clk_frequency) {
    if ((XIicPs_CfgInitialize(iic_instance, ConfigPtr, base_addr) == XST_SUCCESS) && (XIicPs_SetSClk(iic_instance, clk_frequency) == XST_SUCCESS)) {
        xil_printf("Initialization success\n");
        return XST_SUCCESS;
    }
    xil_printf("Initialization failure\n");
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
