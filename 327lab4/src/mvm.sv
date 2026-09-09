/***************************************************/
/* ECE 327: Digital Hardware Systems - Spring 2026 */
/* Lab 4                                           */
/* Matrix Vector Multiplication (MVM) Module       */
/***************************************************/

module mvm # (
    parameter IWIDTH = 8,
    parameter OWIDTH = 32,
    parameter MEM_DATAW = IWIDTH * 8,
    parameter VEC_MEM_DEPTH = 256,
    parameter VEC_ADDRW = $clog2(VEC_MEM_DEPTH),
    parameter VEC_SIZEW = VEC_ADDRW + 1,
    parameter MAT_MEM_DEPTH = 512,
    parameter MAT_ADDRW = $clog2(MAT_MEM_DEPTH),
    parameter MAT_SIZEW = MAT_ADDRW + 1,
    parameter NUM_OLANES = 128
)(
    input clk,
    input rst,
    input [MEM_DATAW-1:0] i_vec_wdata,
    input [VEC_ADDRW-1:0] i_vec_waddr,
    input i_vec_wen,
    input [MEM_DATAW-1:0] i_mat_wdata,
    input [MAT_ADDRW-1:0] i_mat_waddr,
    input [NUM_OLANES-1:0] i_mat_wen,
    input i_start,
    input [VEC_ADDRW-1:0] i_vec_start_addr,
    input [VEC_SIZEW-1:0] i_vec_num_words,
    input [MAT_ADDRW-1:0] i_mat_start_addr,
    input [MAT_SIZEW-1:0] i_mat_num_rows_per_olane,
    output o_busy,
    output [OWIDTH*NUM_OLANES-1:0] o_result,
    output o_valid
);

/******* Your code starts here *******/

// Internal wires for Controller and Memory
logic [VEC_ADDRW-1:0] vec_raddr;
logic [MEM_DATAW-1:0] vec_rdata;

logic [MAT_ADDRW-1:0] mat_raddr;
logic [MEM_DATAW-1:0] mat_rdata [NUM_OLANES-1:0];

logic ctrl_ovalid;
logic accum_first, accum_last;

// 1. Controller Instantiation
ctrl #(
    .VEC_ADDRW(VEC_ADDRW),
    .MAT_ADDRW(MAT_ADDRW),
    .VEC_SIZEW(VEC_SIZEW),
    .MAT_SIZEW(MAT_SIZEW)
) u_ctrl (
    .clk(clk), .rst(rst), .start(i_start),
    .vec_start_addr(i_vec_start_addr), .vec_num_words(i_vec_num_words),
    .mat_start_addr(i_mat_start_addr), .mat_num_rows_per_olane(i_mat_num_rows_per_olane),
    .vec_raddr(vec_raddr),
    .mat_raddr(mat_raddr),
    .accum_first(accum_first),
    .accum_last(accum_last),
    .ovalid(ctrl_ovalid),
    .busy(o_busy)
);

// 2. Vector Memory Instantiation
mem #(
    .DATAW(MEM_DATAW),
    .DEPTH(VEC_MEM_DEPTH)
) vector_mem (
    .clk(clk), .wdata(i_vec_wdata), .waddr(i_vec_waddr), .wen(i_vec_wen),
    .raddr(vec_raddr), .rdata(vec_rdata)
);

// 3. Matrix Memory Instantiation
genvar i;
generate
    for(i = 0; i < NUM_OLANES; i = i + 1) begin : gen_mat_mem
        mem #(
            .DATAW(MEM_DATAW),
            .DEPTH(MAT_MEM_DEPTH)
        ) matrixMem (
            .clk(clk), .wdata(i_mat_wdata), .waddr(i_mat_waddr), .wen(i_mat_wen[i]),
            .raddr(mat_raddr),
            .rdata(mat_rdata[i])
        );
    end
endgenerate

// 4. Latency Alignment Logic
// The memory has a 1-cycle read latency. We delay ctrl_ovalid by 1 cycle 
// so dot8 samples the data exactly when it becomes valid.
logic dot_ivalid;
always_ff @(posedge clk) begin
    if (rst) dot_ivalid <= 1'b0;
    else     dot_ivalid <= ctrl_ovalid; 
end

// OPTIMIZATION FOR 128 OLANES: 
// The dot8 module has a 5-cycle pipeline latency. Combined with the 1-cycle 
// memory delay, the total latency is 6 cycles.
// Instead of instantiating 128 identical shift registers (which wastes resources), 
// we use a SINGLE 6-bit shift register for first/last delay and fan it out to all lanes.
logic [5:0] first_delay;
logic [5:0] last_delay;

always_ff @(posedge clk) begin
    if (rst) begin
        first_delay <= 6'b0;
        last_delay  <= 6'b0;
    end else begin
        first_delay <= {first_delay[4:0], accum_first};
        last_delay  <= {last_delay[4:0], accum_last};
    end
end

// 5. Datapath Instantiation
logic [OWIDTH-1:0] dot_result [NUM_OLANES-1:0];
logic dot_ovalid [NUM_OLANES-1:0]; 
logic [OWIDTH-1:0] accum_result [NUM_OLANES-1:0];
logic accum_ovalid [NUM_OLANES-1:0];

generate
    for (i = 0; i < NUM_OLANES; i++) begin : gen_datapath
        dot8 #( .IWIDTH(IWIDTH), .OWIDTH(OWIDTH) ) u_dot8 (
            .clk(clk), .rst(rst),
            .vec0(vec_rdata),
            .vec1(mat_rdata[i]),
            .ivalid(dot_ivalid),          // Aligned with memory data (1-cycle delay)
            .result(dot_result[i]),
            .ovalid(dot_ovalid[i])
        );

        accum #( .DATAW(OWIDTH), .ACCUMW(OWIDTH) ) u_accum (
            .clk(clk), .rst(rst),
            .data(dot_result[i]),
            .ivalid(dot_ovalid[i]),      
            .first(first_delay[5]),    // Tapped at index 5 (6-cycle delay) to match dot8 output
            .last(last_delay[5]),      // Tapped at index 5 (6-cycle delay) to match dot8 output
            .result(accum_result[i]),
            .ovalid(accum_ovalid[i])
        );
    end
endgenerate

// 6. Output Assignment
genvar j;
generate
    for (j = 0; j < NUM_OLANES; j++) begin : gen_output
        assign o_result[j*OWIDTH +: OWIDTH] = accum_result[j];
    end
endgenerate

// Since all lanes operate in lockstep, checking lane 0's valid signal is sufficient
assign o_valid = accum_ovalid[0]; 

/******* Your code ends here ********/

endmodule