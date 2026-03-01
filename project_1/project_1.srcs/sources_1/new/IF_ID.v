`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/29/2025 09:33:31 AM
// Design Name: 
// Module Name: IF_ID
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


module IF_ID(
    input [15:0] pc_next,
    input [15:0] instr,
    input [15:0] pc,
    input clk,
    output reg [15:0] pc_next_out,
    output reg [1:0] opcode,
    output reg [15:0] pc_out,
    output reg [15:0] instr_out
    );
    always @(posedge clk) 
    begin
        pc_next_out<=pc_next;
        opcode<=instr[1:0];
        pc_out<=pc ;
        instr_out<=instr;
    end
endmodule
