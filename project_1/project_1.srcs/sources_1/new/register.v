`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/01/2026 10:57:32 AM
// Design Name: 
// Module Name: register
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


module register(
    input rw,
    input [2:0] rs1,
    input [2:0] rs2,
    input [2:0] rd,
    input [15:0] wd,
    input clk,
    output [15:0] o1,
    output [15:0] o2
    );
    reg [15:0] local_reg [7:0];
    assign o1 = (rs1 == 3'b000) ? 16'b0 : local_reg[rs1];
    assign o2 = (rs2 == 3'b000) ? 16'b0 : local_reg[rs2];
    always @(posedge clk) begin
        if (  rw && rd != 3'b000) begin
            local_reg[rd] <= wd;
        end
    end
    integer i;
    initial begin
        for (i = 0; i < 8; i = i + 1) local_reg[i] = 0;
    end
endmodule
