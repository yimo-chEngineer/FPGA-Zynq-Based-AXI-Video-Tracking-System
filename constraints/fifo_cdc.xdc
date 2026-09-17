# Registered Gray pointers: bound each crossing to the faster clock period.
# Do not replace these with a blanket asynchronous clock-group exception.
set_max_delay -datapath_only 10.000 -from [get_cells -hier -filter {NAME =~ *async_fifo_0*/*g_wptr_reg* && IS_SEQUENTIAL}] -to [get_cells -hier -filter {NAME =~ *async_fifo_0*/*read_dff/data_temp_reg* && IS_SEQUENTIAL}]
set_bus_skew 10.000 -from [get_cells -hier -filter {NAME =~ *async_fifo_0*/*g_wptr_reg* && IS_SEQUENTIAL}] -to [get_cells -hier -filter {NAME =~ *async_fifo_0*/*read_dff/data_temp_reg* && IS_SEQUENTIAL}]
set_max_delay -datapath_only 10.000 -from [get_cells -hier -filter {NAME =~ *async_fifo_0*/*g_rptr_reg* && IS_SEQUENTIAL}] -to [get_cells -hier -filter {NAME =~ *async_fifo_0*/*write_dff/data_temp_reg* && IS_SEQUENTIAL}]
set_bus_skew 10.000 -from [get_cells -hier -filter {NAME =~ *async_fifo_0*/*g_rptr_reg* && IS_SEQUENTIAL}] -to [get_cells -hier -filter {NAME =~ *async_fifo_0*/*write_dff/data_temp_reg* && IS_SEQUENTIAL}]
