`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/23/2025 09:47:07 PM
// Design Name: 
// Module Name: top
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


module top(
    input sys_clk,
    input sys_rst_n,
    output [3:0] led
    );
    wire sys_rst=!sys_rst_n;
    updown_counter count_1(
     .clk(sys_clk),
     .reset(sys_rst),
     .up_down(1'b1),
     .out(led)
    );
endmodule
