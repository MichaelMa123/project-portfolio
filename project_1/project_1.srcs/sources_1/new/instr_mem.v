`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/28/2025 07:39:58 PM
// Design Name: 
// Module Name: instr_mem
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module instr_mem(
    input clk,
    input [15:0] instr_addr,
    output reg[15:0] instr    
    
    );
    localparam MEM_DEPTH = 1024;
    
    reg [15:0] mem [0 : MEM_DEPTH-1];
    integer i;
    initial begin
        mem[0] = 16'h4501;  // c.addi x1, x1, 1  (example RVC)
        mem[1] = 16'h8082;  // c.j -2 (loop)
        // Fill rest with NOPs (c.nop = 16'h0001)
        for (i = 2; i < MEM_DEPTH; i = i + 1) begin
            mem[i] = 16'h0001;
        end
    end
    always @(*) begin
        instr = mem[instr_addr[10:1]];
    end
endmodule
