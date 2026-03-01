`timescale 1ns / 1ps

module counter_sim;

    // DUT inputs
    reg clk;
    reg reset;
    reg up_down;

    // DUT output
    wire [3:0] out;

    // Instantiate the Unit Under Test (UUT)
    updown_counter uut (
        .clk(clk),
        .reset(reset),
        .up_down(up_down),
        .out(out)
    );

    // Clock generation: 10 ns period (50 MHz)
    always begin
        #5 clk = ~clk; // Toggle every 5 ns
    end

    // Test sequence
    initial begin
        // Initialize
        clk = 0;
        reset = 1;
        up_down = 0; // Start in "down" mode (though reset overrides)

        // Hold reset for 2 clock cycles
        #20 reset = 0;

        // Test counting UP
        up_down = 1;
        #80; // Let it run for 8 clock cycles

        // Test counting DOWN
        up_down = 0;
        #80; // Another 8 clock cycles

        // Switch to UP again mid-sequence
        up_down = 1;
        #40;

        // End simulation
        $stop;
    end

    // Optional: Monitor outputs
    initial begin
        $monitor("Time = %0t | reset = %b | up_down = %b | status_out = %b", $time, reset, up_down, out);
    end

endmodule