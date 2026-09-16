`timescale 1ns / 1ps

module async_fifo(
    input wire  [15:0]  s_axis_tdata,
    input wire          m_axis_tvalid,
    input wire          m_axis_tuser,
    input wire          m_axis_tlast,
    input wire          vdma_axis_tready, //VDMA ready checker
    
    input wire          wclk, //Write and read clocks
    input wire          rclk,
    
    input wire          wrst_n, //Active low resets
    input wire          rrst_n,
    
    // FIFO to VDMA
    output wire [15:0] vdma_axis_tdata,
    output wire        vdma_axis_tuser,
    output wire        vdma_axis_tlast,
    output wire        vdma_axis_tvalid,
    
    output reg          full,
    output reg          empty    
    );
    
    reg  [10:0]  b_wptr; //Wrap bit + Binary pointers
    reg  [10:0]  b_rptr;
    reg  [10:0]  g_wptr; //Wrap bit + Gray code pointers
    reg  [10:0]  g_rptr;
    reg  [10:0]  g_wptr_sync; //Wrap bit + Gray code sync pointers
    reg  [10:0]  g_rptr_sync;    
    reg  [17:0] memory [0:1023];
    wire w_en, r_en;
    wire [10:0] b_wptr_next, g_wptr_next;
    wire [10:0] b_rptr_next, g_rptr_next;
    wire full_next, empty_next;
    
    reg [17:0] data_out; 
    reg        output_valid;
    
    assign vdma_axis_tdata  = data_out[15:0]; //Organize data into outputs used for VDMA
    assign vdma_axis_tlast  = data_out[16];
    assign vdma_axis_tuser  = data_out[17];
    assign vdma_axis_tvalid = output_valid;
    
    assign w_en = m_axis_tvalid && !full; // Writing is enabled when pixel is valid and FIFO is not full
    assign r_en = vdma_axis_tready && !empty; // Reading is enabled when VDMA is ready and FIFO is not empty
    
    // Next cycle pointers for full calculation
    assign b_wptr_next = b_wptr + w_en;
    assign g_wptr_next = b_wptr_next ^ (b_wptr_next >> 1);  
    assign full_next = (g_wptr_next == {~g_rptr_sync[10:9], g_rptr_sync[8:0]});
    // Next cycle pointers for empty caluclation
    assign b_rptr_next = b_rptr + r_en;
    assign g_rptr_next = b_rptr_next ^ (b_rptr_next >> 1);
    assign empty_next = (g_rptr_next == g_wptr_sync);
    
    //Instantiate write and read dff
    double_dff write_dff(.clk(wclk),.rst_n(wrst_n),.data_in(g_rptr),.data_out(g_rptr_sync));
    double_dff read_dff(.clk(rclk),.rst_n(rrst_n),.data_in(g_wptr),.data_out(g_wptr_sync));
    
    
    always @(posedge wclk or negedge wrst_n) begin //Write pointer handler
        if (!wrst_n) begin
            full        <= 1'b0;
            b_wptr      <= 11'b0;
            g_wptr      <= 11'b0;          
        end else begin
            if (w_en) begin
                memory[b_wptr[9:0]] <= {m_axis_tuser, m_axis_tlast, s_axis_tdata}; //Stores tlast, tuser, then pixel data to pointer location
            end
            b_wptr <= b_wptr_next; 
            g_wptr <= g_wptr_next;
            full <= full_next;
        end
    end
    
    
    always @(posedge rclk or negedge rrst_n) begin //Read pointer handler
        if (!rrst_n) begin
            empty       <= 1'b1;
            b_rptr      <= 11'b0;
            g_rptr      <= 11'b0;           
        end else begin
            if (r_en) begin //Begin handling if reading is enabled
                data_out <= memory[b_rptr[9:0]]; //Outputs data at read pointer
            end
            b_rptr <= b_rptr_next; 
            g_rptr <= g_rptr_next;
            empty <= empty_next;
        end
    end
    
    always @(*) begin
        
    end
    
endmodule
