`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/29/2025 09:26:15 AM
// Design Name: 
// Module Name: pc_add
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


module pc_add(
    input [15:0] pc,
    output [15:0] pc_next
    );
    assign pc_next=pc+2;
endmodule
