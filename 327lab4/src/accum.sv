/***************************************************/
/* ECE 327: Digital Hardware Systems - Spring 2026 */
/* Lab 4                                           */
/* Accumulator Module                              */
/***************************************************/

module accum # (
    parameter DATAW = 32,
    parameter ACCUMW = 32
)(
    input  clk,
    input  rst,
    input  signed [DATAW-1:0] data,
    input  ivalid,
    input  first,
    input  last,
    output signed [ACCUMW-1:0] result,
    output ovalid
);

/******* Your code starts here *******/
logic signed [ACCUMW-1:0] result_reg;
logic ovalid_reg;

always_ff @(posedge clk) begin
    if (rst) begin
        result_reg <= '0;
        ovalid_reg <= 1'b0;
    end else begin
        // Default: deassert ovalid to ensure it's a clean pulse
        ovalid_reg <= 1'b0; 
        
        if (ivalid) begin
            if (first) begin
                // Start of a new accumulation: overwrite the register
                result_reg <= data;
            end else begin
                // Continue accumulating
                result_reg <= result_reg + data;
            end
            
            // If this is the last element, assert ovalid for this cycle
            if (last) begin
                ovalid_reg <= 1'b1;
            end
        end
    end
end

assign result = result_reg;
assign ovalid = ovalid_reg;
/******* Your code ends here ********/

endmodule