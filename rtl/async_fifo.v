`timescale 1ns / 1ps

module async_fifo(

    // External pixel capture io
    input wire  [15:0]  m_axis_tdata, //camera pixel data
    input wire          m_axis_tvalid, 
    input wire          m_axis_tuser,
    input wire          m_axis_tlast,
    output wire         m_axis_tready,
    
    //VDMA io
    input wire          vdma_axis_tready, //VDMA ready checker
    output reg  [15:0]  vdma_axis_tdata,
    output reg          vdma_axis_tuser,
    output reg          vdma_axis_tlast,
    output reg          vdma_axis_tvalid,
    
    //Internal FIFO wires
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 wclk CLK" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF m_axis, ASSOCIATED_RESET wrst_n" *)
    input wire          wclk, //Write and read clocks
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 rclk CLK" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF vdma_axis, ASSOCIATED_RESET rrst_n" *)
    input wire          rclk,
    input wire          wrst_n, //Active low resets
    input wire          rrst_n,
    
    output reg          full,
    output reg          empty    
    );
    
    reg  [10:0]  b_wptr; //Wrap bit + Binary pointers
    reg  [10:0]  b_rptr;
    reg  [10:0]  g_wptr; //Wrap bit + Gray code pointers
    reg  [10:0]  g_rptr;
    wire  [10:0]  g_wptr_sync; //Wrap bit + Gray code sync pointers
    wire  [10:0]  g_rptr_sync;    
    (* ram_style = "block" *) reg [17:0] memory [0:1023];
    
    wire w_en, r_en;
    wire [10:0] b_wptr_next, g_wptr_next;
    wire [10:0] b_rptr_next, g_rptr_next;
    wire full_next, empty_next;

    assign m_axis_tready = wrst_n && !full;
    assign w_en = m_axis_tvalid && m_axis_tready;
    assign r_en = rrst_n && !empty && (!vdma_axis_tvalid || vdma_axis_tready);
    
    // Next cycle pointers for calculating full
    assign b_wptr_next = b_wptr + w_en;
    assign g_wptr_next = b_wptr_next ^ (b_wptr_next >> 1);  
    assign full_next = (g_wptr_next == {~g_rptr_sync[10:9], g_rptr_sync[8:0]});
    // Next cycle pointers for calculating empty 
    assign b_rptr_next = b_rptr + r_en;
    assign g_rptr_next = b_rptr_next ^ (b_rptr_next >> 1);
    assign empty_next = (g_rptr_next == g_wptr_sync);
    
    //Instantiate write and read dff
    double_dff write_dff(.clk(wclk),.rst_n(wrst_n),.data_in(g_rptr),.data_out(g_rptr_sync));
    double_dff read_dff(.clk(rclk),.rst_n(rrst_n),.data_in(g_wptr),.data_out(g_wptr_sync));
    
    // Keep RAM ports free of asynchronous reset for dual-clock BRAM inference.
    always @(posedge wclk) begin
        if (w_en)
            memory[b_wptr[9:0]] <= {m_axis_tuser, m_axis_tlast, m_axis_tdata};
    end

    always @(posedge rclk) begin
        if (r_en)
            {vdma_axis_tuser, vdma_axis_tlast, vdma_axis_tdata}
                <= memory[b_rptr[9:0]];
    end

    always @(posedge wclk or negedge wrst_n) begin //Write pointer handler
        if (!wrst_n) begin
            full        <= 1'b0;
            b_wptr      <= 11'b0;
            g_wptr      <= 11'b0;          
        end else begin
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
            vdma_axis_tvalid <= 1'b0;           
        end else begin
            if (r_en) begin //Begin handling if reading is enabled
                vdma_axis_tvalid <= 1'b1;
            end else if (vdma_axis_tvalid && vdma_axis_tready) begin 
                vdma_axis_tvalid <= 1'b0;
            end
            b_rptr <= b_rptr_next; 
            g_rptr <= g_rptr_next;
            empty <= empty_next;
        end
    end
  
    
endmodule
