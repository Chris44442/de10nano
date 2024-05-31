create_clock -period "50.0 MHz" [get_ports FPGA_CLK1_50]
create_clock -period "1 MHz" [get_ports HPS_I2C0_SCLK]
create_clock -period "1 MHz" [get_ports HPS_I2C1_SCLK]
create_clock -period "48 MHz" [get_ports HPS_USB_CLKOUT]
derive_pll_clocks
derive_clock_uncertainty