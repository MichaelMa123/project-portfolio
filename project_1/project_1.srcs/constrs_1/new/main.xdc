# Clock
set_property PACKAGE_PIN R4  [get_ports sys_clk]
set_property IOSTANDARD LVCMOS33 [get_ports sys_clk]
create_clock -period 10.000 -name sys_clk [get_ports sys_clk]

# Reset (active-low)
set_property PACKAGE_PIN U7  [get_ports sys_rst_n]
set_property IOSTANDARD LVCMOS33 [get_ports sys_rst_n]

# LEDs
set_property PACKAGE_PIN V9  [get_ports {led[0]}]
set_property PACKAGE_PIN Y8  [get_ports {led[1]}]
set_property PACKAGE_PIN Y7  [get_ports {led[2]}]
set_property PACKAGE_PIN W7  [get_ports {led[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports led]