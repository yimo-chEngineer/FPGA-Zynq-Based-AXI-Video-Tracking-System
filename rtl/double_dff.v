`timescale 1ns / 1ps

module double_dff(
    input wire clk,
    input wire rst_n,
    input wire  [11:0]  data_in,
    
    output reg  [11:0]  data_out
    );
    
    reg  [11:0]  data_temp;
    
    always @(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            data_temp <= 1'b0;
            data_out  <= 1'b0;
        end else begin
            data_temp <= data_in;
            data_out  <= data_temp;
        end
    end
    
endmodule
