`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/23/2025 05:15:41 PM
// Design Name: 
// Module Name: mem_8kb
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

module mem_8kb
    #(
        parameter MEM_SIZE = 8192,      // 8KB = 8192 bytes
        parameter MEMBER_SIZE = 8       // 8 bits per location
    )
    (
        input clk,
        input [12:0] address,           // 8192 = 2^13 → need 13 bits (not 16!)
        input we,                       // write enable: 1 = write, 0 = read
        input [7:0] write_data,
        output [7:0] read_data
    );

    reg [7:0] mem [0:MEM_SIZE-1];

    // Combinational read
    assign read_data = mem[address];

    // Synchronous write
    always @(posedge clk) begin
        if (we)
            mem[address] <= write_data;
    end
endmodule
