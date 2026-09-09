/***************************************************/
/* ECE 327: Digital Hardware Systems - Spring 2026 */
/* Lab 4                                           */
/* 8-Lane Dot Product Module                       */
/***************************************************/

module dot8 # (
    parameter IWIDTH = 8,
    parameter OWIDTH = 32
)(
    input clk,
    input rst,
    input signed [8*IWIDTH-1:0] vec0,
    input signed [8*IWIDTH-1:0] vec1,
    input ivalid,
    output signed [OWIDTH-1:0] result,
    output ovalid
);

/******* Your code starts here *******/
logic signed [IWIDTH*2-1:0] multi_result [7:0];
logic signed [IWIDTH*2:0] add_result_l1 [3:0];
logic signed [IWIDTH*2+1:0] add_result_l2 [1:0];
logic signed [OWIDTH-1:0] add_result_l3;
logic signed [IWIDTH-1:0] input_1_reg [7:0];
logic signed [IWIDTH-1:0] input_2_reg [7:0];
logic valid_array[4:0];
integer i;
always_ff @(posedge clk) begin : dot_prod
    
    if (rst) begin
        for (i = 0; i < 8; i++) begin
            input_1_reg[i] <= '0;
            input_2_reg[i] <= '0;
        end
        for (i = 0; i < 5; i++) begin
            valid_array[i] <= 1'b0;
        end
    end 
    else begin
        
        // cycle1
        if (ivalid) begin
            for (i = 0; i < 8; i++) begin
                input_1_reg[i] <= vec0[i*IWIDTH +: IWIDTH];
                input_2_reg[i] <= vec1[i*IWIDTH +: IWIDTH];
            end
        end
        valid_array[0] <= ivalid;
        // cycle2
        if (valid_array[0]) begin
            for (i = 0; i < 8; i++) begin
                multi_result[i] <= input_1_reg[i] * input_2_reg[i];
            end
        end
        valid_array[1] <= valid_array[0];

        // cycle3
        if (valid_array[1]) begin
            for (i = 0; i < 4; i++) begin
                add_result_l1[i] <= multi_result[i*2+1] + multi_result[i*2];
            end
        end
        valid_array[2] <= valid_array[1];

        // cycle4
        if (valid_array[2]) begin
            for (i = 0; i < 2; i++) begin
                add_result_l2[i] <= add_result_l1[i*2+1] + add_result_l1[i*2];
            end
        end
        valid_array[3] <= valid_array[2];

        // cycle5
        if (valid_array[3]) begin
            add_result_l3 <= add_result_l2[1] + add_result_l2[0];
        end
        valid_array[4] <= valid_array[3];
    end
end
assign ovalid = valid_array[4];
assign result = add_result_l3;


/******* Your code ends here ********/

endmodule