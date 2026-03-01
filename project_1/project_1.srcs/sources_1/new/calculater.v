`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/22/2025 07:10:23 PM
// Design Name: 
// Module Name: calculater
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


module calculater(
        input [7:0] a,
        input [7:0] b,
        input [1:0] op,
        input clk,
        output reg [15:0] out 
    );
    parameter Rest=2'b00, Add=2'b01, Muiltiply=2'b11, Subtract=2'b10;
    wire [8:0] add_result;
    adder_8b adder_0 (.a(a), .b(b), .out(add_result));
    always@(posedge clk)
    
    begin
        case(op)
            Add:
                out<=add_result;    
            Muiltiply:
                out<=1'b0;
            Subtract:
                out<=1'b0;
            default:
                out<=1'b0;
        endcase    
    end
endmodule
