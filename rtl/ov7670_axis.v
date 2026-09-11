`timescale 1ns/1ps
module ov7670_axis #(
    parameter integer WIDTH = 640,
    parameter integer HEIGHT = 480
) (
    input  wire        pclk,        // Camera pixel clock; sample camera signals on each rising edge
    input  wire        aresetn,     // Active-low reset: 0 resets the capture module
    input  wire        vsync,       // Camera frame synchronization; identifies the frame boundary
    input  wire        href,        // High while the camera is sending valid bytes for an active line
    input  wire [7:0]  data,        // 8-bit camera data; two consecutive bytes form one RGB565 pixel

    output reg  [15:0] axis_tdata,  // Complete 16-bit RGB565 output pixel
    output reg         axis_tvalid, // High when axis_tdata contains a valid pixel
    input  wire        axis_tready, // High when the next AXI block is ready to accept a pixel
    output reg         axis_tuser,  // High with the first pixel of each frame
    output reg         axis_tlast,  // High with the last pixel of each line

    output reg         overflow    // High if a camera pixel arrives when it cannot be stored
);

    reg [7:0] first_byte; // Stores the first camera byte of the current RGB565 pixel
    reg       byte_phase; // 0: waiting for first byte; 1: waiting for second byte
    reg [9:0] x;          // Horizontal pixel counter; 10 bits represent positions 0–639
    reg [8:0] y;          // Vertical line counter; 9 bits represent lines 0–479
    reg       armed;      // High after a VSYNC boundary has been observed; prevents capture from starting mid-frame 
        
    always @(posedge pclk or negedge aresetn) begin
        if (!aresetn) begin //If reset is active
            axis_tdata  <= 16'b0;
            axis_tvalid <= 1'b0;
            axis_tuser  <= 1'b0;
            axis_tlast  <= 1'b0;
            
            first_byte  <= 1'b0;
            byte_phase  <= 1'b0;
            
            overflow      <= 1'b0;
            
            x <= 1'b0;
            y <= 1'b0;
            armed <= 1'b0;
        end else begin
            if (axis_tvalid && axis_tready) begin //Previous AXI Pixel was accepted 
                axis_tvalid <= 1'b0; //Reset to prepare for new pixel
                //Reset last or first indicators for next pixel
                axis_tuser <= 1'b0;
                axis_tlast <=1'b0; 
            end 
            if (vsync) begin //Prepare for a new frame
                byte_phase <= 1'b0;
                x <= 1'b0;
                y <= 1'b0;
                armed <= 1'b1;
            end else if (href && armed) begin  
                if (byte_phase == 1'b0) begin //Capture the first byte of a pixel
                    first_byte <= data;
                    byte_phase <= 1'b1;
                end else begin //Combine first and second byte for the pixel
                    //Reset values for the next pixel
                    byte_phase  <= 1'b0;
                    if (x == WIDTH - 1 && y == HEIGHT-1) begin //Reset conditions if on the last pixel
                        x <= 1'b0;
                        y <= 1'b0;
                        armed <= 1'b0;
                    end else if (x == WIDTH - 1) begin //Confirm on last column; Move onto the next row, reset x
                        x <= 1'b0;
                        y <= y + 1;
                    end else 
                    x <= x + 1;
                    if (!axis_tvalid || axis_tready) begin //Checks if pixel is valid and AXI is ready
                        axis_tdata  <= {first_byte, data};
                        axis_tvalid <= 1'b1;
                        //Check if pixel was on last column or first
                        axis_tuser  <= (x == 0 && y == 0); 
                        axis_tlast  <= (x == WIDTH - 1);
                    end else if (!axis_tready && axis_tvalid) begin 
                        overflow <= 1'b1;
                    end
                end 
            end else begin
                byte_phase <= 0;
            end   
        end
    end
endmodule
