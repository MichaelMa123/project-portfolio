`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/05/2026 09:56:45 AM
// Design Name: 
// Module Name: ALU
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: Combinational 16-bit ALU with common RISC-V-like operations
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module ALU(
    input [15:0] in_1,
    input [15:0] in_2,
    input [2:0] ALU_OP,
    output reg [15:0] alu_result,
    output zero
);

parameter 
    ALU_ADD  = 3'b000,  // A + B
    ALU_SUB  = 3'b001,  // A - B
    ALU_AND  = 3'b010,  // A & B
    ALU_OR   = 3'b011,  // A | B
    ALU_XOR  = 3'b100,  // A ^ B
    ALU_SLL  = 3'b101,  // A << B[3:0]
    ALU_SLT  = 3'b110,  // (A < B) ? 1 : 0 (signed, sign-extended result)
    ALU_SRA  = 3'b111;  // A >>> B[3:0] (arithmetic right shift)

wire [3:0] shift_amount = in_2[3:0];

// Zero flag is purely combinational
assign zero = (alu_result == 16'b0);

always @(*) begin
    case(ALU_OP)
        ALU_ADD: 
            alu_result = in_1 + in_2;
            
        ALU_SUB: 
            alu_result = in_1 - in_2;
            
        ALU_AND: 
            alu_result = in_1 & in_2;
            
        ALU_OR: 
            alu_result = in_1 | in_2;
            
        ALU_XOR: 
            alu_result = in_1 ^ in_2;
            
        ALU_SLL: 
            alu_result = in_1 << shift_amount;
            
        ALU_SLT: begin
            if ($signed(in_1) < $signed(in_2))
                alu_result = 16'h0001;
            else
                alu_result = 16'h0000;
        end
        
        ALU_SRA: begin
            alu_result = $signed(in_1) >>> shift_amount;
//            case(shift_amount)
//                0:  alu_result = in_1;
//                1:  alu_result = {in_1[15], in_1[15:1]};
//                2:  alu_result = {{2{in_1[15]}}, in_1[15:2]};
//                3:  alu_result = {{3{in_1[15]}}, in_1[15:3]};
//                4:  alu_result = {{4{in_1[15]}}, in_1[15:4]};
//                5:  alu_result = {{5{in_1[15]}}, in_1[15:5]};
//                6:  alu_result = {{6{in_1[15]}}, in_1[15:6]};
//                7:  alu_result = {{7{in_1[15]}}, in_1[15:7]};
//                8:  alu_result = {{8{in_1[15]}}, in_1[15:8]};
//                9:  alu_result = {{9{in_1[15]}}, in_1[15:9]};
//                10: alu_result = {{10{in_1[15]}}, in_1[15:10]};
//                11: alu_result = {{11{in_1[15]}}, in_1[15:11]};
//                12: alu_result = {{12{in_1[15]}}, in_1[15:12]};
//                13: alu_result = {{13{in_1[15]}}, in_1[15:13]};
//                14: alu_result = {{14{in_1[15]}}, in_1[15:14]};
//                15: alu_result = {{16{in_1[15]}}};
//                default: alu_result = {{16{in_1[15]}}};
//            endcase
        end
        
        default: 
            alu_result = in_1 + in_2;  // Default to ADD
    endcase
end

endmodule