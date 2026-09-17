# Provisional 24 MHz upper-bound PCLK for capture bring-up.
# Confirm programmed sensor PCLK frequency and source-synchronous input delays
# before treating board input timing as closed. No input paths are false-pathed.
create_clock -name camera_pclk -period 41.667 [get_ports pclk]
