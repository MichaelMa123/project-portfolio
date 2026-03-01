`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/21/2025 04:57:28 PM
// Design Name: 
// Module Name: sw_to_led
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


module sw_to_led(
    input sw1,
    input sw2,
    input clk,
    output reg [1:0] out
    );
    always@(posedge clk)
        begin
            if(sw1&sw2)
                out <=2'b11;
            else if(sw1&!sw2)
                out <=2'b10;
            else if(!sw1&sw2)
                out <=2'b01;
            else
                out <=2'b00;
        end     
endmodule
