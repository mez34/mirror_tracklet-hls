# generate project script
# vivado_hls -f <filename>
# vivado_hls -p generic_proj
# WARNING: this will wipe out the original project by the same name

# set clock 250 MHz
set clockperiod 4        

# delete old project
delete_project generic_proj
# make new project
open_project generic_proj
set_top GenericTop

# source files
add_files ../TrackletAlgorithm/GenericTop_v5.cpp -cflags "-std=c++11"
add_files -tb -cflags "-I ../TrackletAlgorithm -std=c++11" ../TestBenches/Generic_test.cpp

# data files
add_files -tb ../emData 
#/MC/MC_L3PHIC/

# solutions
open_solution "solution1"
source set_fpga.tcl
create_clock -period $clockperiod -name default

csim_design
csynth_design
#cosim_design 
#cosim design -O -mflags "-j9 -k"
#export_design -format ip_catalog


# exit vivado_hls
quit
