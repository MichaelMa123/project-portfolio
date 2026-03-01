`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/21/2025 03:19:36 PM
// Design Name: 
// Module Name: updown_counter
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


module updown_counter(
    input clk,
    input reset,
    input up_down,
    output reg [3:0] out
    );
    reg [1:0] status=2'b00;
    parameter LED_0=2'b00, LED_1=2'b01, LED_2=2'b11, LED_3=2'b10;
    always@(posedge clk)
    begin
        case(status)
            LED_0:
                out<=4'b0001;
            LED_1:
                out<=4'b0010;
            LED_2:
                out<=4'b0100;
            default:
                out<=4'b1000; 
         endcase    
    end
    always@(posedge clk or posedge reset)
    begin
        if(reset)
            status<=LED_0;
        else
            case(status)
                LED_0:
                    if(up_down)
                        status<=LED_1;
                    else
                        status<=LED_3;
                LED_1:
                    if(up_down)
                        status<=LED_2;
                    else
                        status<=LED_0;
                LED_2:
                    if(up_down)
                        status<=LED_3;
                    else
                        status<=LED_1;
                default:
                    if(up_down)
                        status<=LED_0;
                    else
                        status<=LED_2;
            endcase 
                
    end
endmodule
