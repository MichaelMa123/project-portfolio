/***************************************************/
/* ECE 327: Digital Hardware Systems - Spring 2026 */
/* Lab 4                                           */
/* MVM Control FSM                                 */
/***************************************************/

module ctrl # (
    parameter VEC_ADDRW = 8,
    parameter MAT_ADDRW = 9,
    parameter VEC_SIZEW = VEC_ADDRW + 1,
    parameter MAT_SIZEW = MAT_ADDRW + 1
)(
    input  clk,
    input  rst,
    input  start,
    input  [VEC_ADDRW-1:0] vec_start_addr,
    input  [VEC_SIZEW-1:0] vec_num_words,
    input  [MAT_ADDRW-1:0] mat_start_addr,
    input  [MAT_SIZEW-1:0] mat_num_rows_per_olane,
    output [VEC_ADDRW-1:0] vec_raddr,
    output [MAT_ADDRW-1:0] mat_raddr,
    output accum_first,
    output accum_last,
    output ovalid,
    output busy
);

/******* Your code starts here *******/

enum {IDLE, COMPUTE} state, next_state;

logic [VEC_ADDRW-1:0] vec_raddr_val, next_vec_raddr;
logic [MAT_ADDRW-1:0] mat_raddr_val, next_mat_raddr;
logic [MAT_SIZEW-1:0] row_count, next_row_count;

logic accum_first_val, next_accum_first;
logic accum_last_val, next_accum_last;
logic ovalid_val, next_ovalid;
logic busy_val, next_busy;

assign vec_raddr = vec_raddr_val;
assign mat_raddr = mat_raddr_val;
assign accum_first = accum_first_val;
assign accum_last = accum_last_val;
assign ovalid = ovalid_val;
assign busy = busy_val;

always_ff @(posedge clk) begin
    if (rst) begin
        state           <= IDLE;
        vec_raddr_val   <= 'd0;
        mat_raddr_val   <= 'd0;
        row_count       <= 'd0;
        accum_first_val <= 1'b0;
        accum_last_val  <= 1'b0;
        ovalid_val      <= 1'b0;
        busy_val        <= 1'b0;
    end else begin
        state           <= next_state;
        vec_raddr_val   <= next_vec_raddr;
        mat_raddr_val   <= next_mat_raddr;
        row_count       <= next_row_count;
        accum_first_val <= next_accum_first;
        accum_last_val  <= next_accum_last;
        ovalid_val      <= next_ovalid;
        busy_val        <= next_busy;
    end
end

always_comb begin
    next_state       = state;
    next_vec_raddr   = vec_raddr_val;
    next_mat_raddr   = mat_raddr_val;
    next_row_count   = row_count;
    next_accum_first = 1'b0;
    next_accum_last  = 1'b0;
    next_ovalid      = 1'b0;
    next_busy        = 1'b0;

    case (state)
        IDLE: begin
            if (start) begin
                next_state       = COMPUTE;
                next_vec_raddr   = vec_start_addr;
                next_mat_raddr   = mat_start_addr;
                next_row_count   = 'd0;
                next_accum_first = 1'b1;
                next_ovalid      = 1'b1;
                next_busy        = 1'b1;
                
                if (vec_num_words == 1) begin
                    next_accum_last = 1'b1;
                end
            end
        end
        
        COMPUTE: begin
            next_ovalid = 1'b1;
            next_busy   = 1'b1;
            
            next_vec_raddr = vec_raddr_val + 1;
            next_mat_raddr = mat_raddr_val + 1;
            
            if (vec_raddr_val == (vec_start_addr + vec_num_words - 2)) begin
                next_accum_last = 1'b1;
            end
            
            if (vec_raddr_val == (vec_start_addr + vec_num_words - 1)) begin
                next_vec_raddr = vec_start_addr;
                next_row_count = row_count + 1;
                
                if (row_count == (mat_num_rows_per_olane - 1)) begin
                    next_state       = IDLE;
                    next_vec_raddr   = 'd0;
                    next_mat_raddr   = 'd0;
                    next_ovalid      = 1'b0;
                    next_busy        = 1'b0;
                    next_accum_first = 1'b0;
                    next_accum_last  = 1'b0;
                end else begin
                    next_accum_first = 1'b1;
                    if (vec_num_words == 1) begin
                        next_accum_last = 1'b1;
                    end
                end
            end
        end
        
        default: next_state = IDLE;
    endcase
end
/******* Your code ends here ********/
endmodule