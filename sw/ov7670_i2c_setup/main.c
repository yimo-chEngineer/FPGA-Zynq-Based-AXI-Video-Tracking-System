#include "xparameters.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "sleep.h"

// Hardware Definitions
#define IIC_BASE_ADDR        XPAR_XIICPS_0_BASEADDR
#define OV7670_I2C_ADDR      0x21    // 7-bit standard address (0x42 shifted right by 1)
#define SCCB_SCLK_RATE       100000  // 100 kHz is mandatory for SCCB stability

XIicPs Iic;

// SCCB Register Write: [Reg Addr] -> [Data Byte]
int sccb_write_reg(u8 reg_addr, u8 data) {
    u8 write_buf[2];
    write_buf[0] = reg_addr;
    write_buf[1] = data;

    // Send 2 bytes over the EMIO pins to the OV7670
    int Status = XIicPs_MasterSendPolled(&Iic, write_buf, 2, OV7670_I2C_ADDR);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Wait until the bus goes idle
    while (XIicPs_BusIsBusy(&Iic));
    usleep(1000); // Small delay to let camera write internal EEPROM/registers
    return XST_SUCCESS;
}

// SCCB Register Read: Write Reg Addr -> Read Data Byte
int sccb_read_reg(u8 reg_addr, u8 *data) {
    int Status;

    // Phase 1: Tell the camera which register address we want to read
    Status = XIicPs_MasterSendPolled(&Iic, &reg_addr, 1, OV7670_I2C_ADDR);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    while (XIicPs_BusIsBusy(&Iic));

    // Phase 2: Read the byte back
    Status = XIicPs_MasterRecvPolled(&Iic, data, 1, OV7670_I2C_ADDR);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    while (XIicPs_BusIsBusy(&Iic));

    return XST_SUCCESS;
}

typedef struct {
    u8 reg;
    u8 val;
} ov7670_reg_t;

static const ov7670_reg_t ov7670_vga_rgb565[] = {
    {0x12, 0x80}, // COM7: Reset all registers
    {0x11, 0x01}, // CLKRC: Internal Clock Pre-scaler
    {0x12, 0x04}, // COM7: VGA + RGB Output
    {0x40, 0xD0}, // COM15: Full range [16-255], RGB565 format
    {0x8C, 0x00}, // RGB444: Disable
    {0x17, 0x16}, // HSTART
    {0x18, 0x04}, // HSTOP
    {0x32, 0x24}, // HREF
    {0x19, 0x02}, // VSTRT
    {0x1A, 0x7A}, // VSTOP
    {0x03, 0x0A}, // VREF
    {0xFF, 0xFF}  // End of Configuration Flag
};

int configure_ov7670(void) {
    int i = 0;
    xil_printf("Configuring OV7670 for VGA RGB565...\r\n");

    while (ov7670_vga_rgb565[i].reg != 0xFF) {
        u8 reg = ov7670_vga_rgb565[i].reg;
        u8 val = ov7670_vga_rgb565[i].val;

        if (sccb_write_reg(reg, val) != XST_SUCCESS) {
            xil_printf("Failed to write Reg: 0x%02X\r\n", reg);
            return XST_FAILURE;
        }

        // Small delay after soft-reset command
        if (reg == 0x12 && (val & 0x80)) usleep(100000); 
        i++;
    }
    return XST_SUCCESS;
}

int main() {
    int Status;
    XIicPs_Config *Config;
    u8 pid = 0x00, ver = 0x00;

    xil_printf("\r\n--- Initializing Zynq I2C for OV7670 SCCB ---\r\n");

    Config = XIicPs_LookupConfig(IIC_BASE_ADDR);
    if (NULL == Config) return XST_FAILURE;

    Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Set 100kHz SCLK for SCCB
    XIicPs_SetSClk(&Iic, SCCB_SCLK_RATE);

    // Reset OV7670 via Software (COM7 register 0x12 = 0x80)
    xil_printf("Resetting OV7670...\r\n");
    sccb_write_reg(0x12, 0x80);
    usleep(100000); // 100ms delay after soft reset

    // Ping Camera: Read Product ID (PID) Reg 0x0A and Version Reg 0x0B
    // Default OV7670 PID should return 0x76, VER should return 0x70
    if (sccb_read_reg(0x0A, &pid) == XST_SUCCESS && sccb_read_reg(0x0B, &ver) == XST_SUCCESS) {
        xil_printf("SUCCESS! Connected to OV7670. Product ID: 0x%02X, Version: 0x%02X\r\n", pid, ver);
    } else {
        xil_printf("ERROR: Could not communicate with OV7670 over I2C/SCCB.\r\n");
    }
    if (configure_ov7670() == XST_SUCCESS) {
        xil_printf("OV7670 configured successfully! Camera is outputting data.\r\n");
    }

    while(1);
    return XST_SUCCESS;
}