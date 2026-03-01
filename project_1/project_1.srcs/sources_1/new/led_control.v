`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/24/2025 10:24:01 AM
// Design Name: 
// Module Name: led_control
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


module led_control (
    input  led0,
    input  led1,
    input  led2,
    input  led3,
    output [3:0] led
);
    assign led = {led3, led2, led1, led0};  // MSB → LSB
endmodule
