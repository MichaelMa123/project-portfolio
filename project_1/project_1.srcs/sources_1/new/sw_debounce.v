`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/21/2025 05:10:45 PM
// Design Name: 
// Module Name: sw_debounce
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


module sw_debounce(
    input sw_in,
    input clk,
    output reg sw_out
    );
    reg [0:0] sw_stat =1'b0;
    reg pause=1'b0;
    reg [4:0] counter =5'b00000;
    always@(posedge clk)
    begin
        if(!(sw_in==sw_stat)&!pause)
        begin
            counter <= 5'b00000;
            pause<=1;
        end
        if(pause)
        begin
            counter<=counter+1;
            if(counter==5'b11111)begin
                pause<=0;
                sw_out<=sw_in;
                sw_stat<=sw_in;
            end
        end
    end
endmodule
