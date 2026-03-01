`timescale 1ns / 1ps

module tb_top;

    reg sys_clk = 0;
    reg sys_rst_n = 1;  // <-- Never reset! Always deasserted (active-low reset = 1)
    wire [3:0] led;

    top uut (
        .sys_clk(sys_clk),
        .sys_rst_n(sys_rst_n),
        .led(led)
    );

    // 50 MHz clock: 20 ns period
    always #10 sys_clk = ~sys_clk;

    initial begin
        // No reset pulse - just run
        #200;  // Simulate for 200 ns (10 clock cycles)
        $finish;
    end

    initial $monitor("Time = %0t | led = %b", $time, led);

endmodule