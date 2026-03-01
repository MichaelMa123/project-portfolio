`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/29/2025 07:55:49 PM
// Design Name: 
// Module Name: control_unit
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 1.1 - ALU opcode optimization and RISC-V compliance fixes
//////////////////////////////////////////////////////////////////////////////////


module control_unit(
    input [1:0] opcode,
    input [2:0] func3,
    input [1:0] func2,
    input clk,
    output reg [2:0]  ALU_OP,    // Reduced to 3 bits (8 operations)
    output reg branch,
    output reg mem_read,
    output reg mem_to_reg,
    output reg mem_write,
    output reg alu_src,
    output reg reg_write
    );
    
    // ALU Operation Constants (3-bit encoding)
    parameter 
        ALU_ADD  = 3'b000,  // A + B
        ALU_SUB  = 3'b001,  // A - B
        ALU_AND  = 3'b010,  // A & B
        ALU_OR   = 3'b011,  // A | B
        ALU_XOR  = 3'b100,  // A ^ B
        ALU_SLL  = 3'b101,  // A << B[3:0]
        ALU_SLT  = 3'b110,  // (A < B) ? 1 : 0 (signed)
        ALU_SRA  = 3'b111;  // A >>> B[3:0] (arithmetic shift)
    
    // Opcode constants
    parameter I_type=2'b00, R_type=2'b01, S_type=2'b11, B_type=2'b10;
    
    // Function code constants
    parameter ADDI=3'b000, XORI=3'b001, ORI=3'b011, ANDI=3'b010,
              SLLI=3'b100, SRLI=3'b101, SRAI=3'b110, SLTI=3'b111;
    
    parameter ADD=2'b00, SUB=2'b01, SLL=2'b11, SLT=2'b10;  // R-type func2
    
    parameter SB=3'b000, SH=3'b001, LB=3'b011, LH=3'b010;  // S/B-type func3
    
    parameter BEQ=3'b000, BNE=3'b001, BLT=3'b111, BGE=3'b110;  // B-type func3

always @(*)
begin
    // Default values (prevent latch inference)
    ALU_OP      = ALU_ADD;  // Default to ADD
    branch      = 1'b0;
    mem_read    = 1'b0;
    mem_to_reg  = 1'b0;
    mem_write   = 1'b0;
    alu_src     = 1'b0;  // 0 = reg, 1 = immediate
    reg_write   = 1'b0;

    case(opcode)
        R_type:
        begin
            // R-type always writes back to register file
            reg_write   = 1'b1;
            alu_src     = 1'b0;  // Use register inputs
            mem_read    = 1'b0;
            mem_write   = 1'b0;
            mem_to_reg  = 1'b0;
            branch      = 1'b0;
            
            // Decode R-type function
            case(func2)
                ADD:   ALU_OP = ALU_ADD;
                SUB:   ALU_OP = ALU_SUB;
                SLL:   ALU_OP = ALU_SLL;
                SLT:   ALU_OP = ALU_SLT;  // Added missing SLT operation
                default: ALU_OP = ALU_ADD; // Safe default
            endcase
        end
        
        S_type:
        begin
            alu_src     = 1'b0;  // Base address from register
            reg_write   = 1'b0;  // Default: no register write
            branch      = 1'b0;
            ALU_OP      = ALU_ADD;  // Address calculation = base + offset
            
            case(func3)
                SB, SH:  // Store instructions
                begin
                    mem_write = 1'b1;
                    mem_read  = 1'b0;
                    mem_to_reg= 1'b0;
                    reg_write = 1'b0;
                end
                
                LB, LH:  // Load instructions
                begin
                    mem_write = 1'b0;
                    mem_read  = 1'b1;
                    mem_to_reg= 1'b1;
                    reg_write = 1'b1;  // Load result to register
                end
                
                default: // Safe defaults
                begin
                    mem_write = 1'b0;
                    mem_read  = 1'b0;
                end
            endcase
        end
        
        B_type:
        begin
            branch      = 1'b1;
            alu_src     = 1'b0;  // Compare two registers
            reg_write   = 1'b0;
            mem_read    = 1'b0;
            mem_write   = 1'b0;
            mem_to_reg  = 1'b0;
            
            // Critical fix: Different ALU ops for different branch types
            case(func3)
                BEQ, BNE: ALU_OP = ALU_SUB;  // Equality check via subtraction
                BLT, BGE: ALU_OP = ALU_SLT;  // Signed comparison
                default:  ALU_OP = ALU_SUB;
            endcase
        end
        
        I_type:  // Default case is I-type (immediate instructions)
        begin
            reg_write   = 1'b1;
            alu_src     = 1'b1;  // Use immediate value
            mem_read    = 1'b0;
            mem_write   = 1'b0;
            mem_to_reg  = 1'b0;
            branch      = 1'b0;
            
            case(func3)
                ADDI:   ALU_OP = ALU_ADD;
                XORI:   ALU_OP = ALU_XOR;
                ORI:    ALU_OP = ALU_OR;
                ANDI:   ALU_OP = ALU_AND;
                SLLI:   ALU_OP = ALU_SLL;
                SRLI:   ALU_OP = ALU_SRA;  // Logical shift = arithmetic with positive sign
                SRAI:   ALU_OP = ALU_SRA;  // Arithmetic shift (sign-extended)
                SLTI:   ALU_OP = ALU_SLT;
                default: ALU_OP = ALU_ADD;
            endcase
        end
        
        default: // Safe defaults for undefined opcodes
        begin
            ALU_OP      = ALU_ADD;
            branch      = 1'b0;
            mem_read    = 1'b0;
            mem_to_reg  = 1'b0;
            mem_write   = 1'b0;
            alu_src     = 1'b0;
            reg_write   = 1'b0;
        end
    endcase
end

endmodule
