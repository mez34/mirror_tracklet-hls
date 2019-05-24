open_project -reset prmemc
set_top PRMEMC
add_files ../TrackletAlgorithm/PRMEMC.cpp -cflags "-std=c++11"
add_files -tb ../TestBenches/PRMEMC_test.cpp -cflags "-I../TrackletAlgorithm -std=c++11"
add_files -tb ../emData/Test
open_solution -reset "solution1"
set_part {xcvu7p-flvb2104-2-e} -tool vivado
create_clock -period 4 -name default
csim_design
#csynth_design
#cosim_design
#export_design -rtl verilog -format ip_catalog

#exit
