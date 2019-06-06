#!/usr/bin/env python3
# Francesco Conti <f.conti@unibo.it>
#
# Copyright (C) 2016-2018 ETH Zurich, University of Bologna.
# All rights reserved.

from ipstools_cfg import *

execute("mkdir -p vsim/vcompile/ips")
execute("rm -rf vsim/vcompile/ips/*")
execute("mkdir -p vsim/vcompile/rtl")
execute("rm -rf vsim/vcompile/rtl/*")
execute("mkdir -p vsim/vcompile/tb")
execute("rm -rf vsim/vcompile/tb/*")

# creates an IPApproX database
ipdb = ipstools.IPDatabase(rtl_dir='rtl', ips_dir='ips', vsim_dir='vsim', load_cache=True)

# generate ModelSim/QuestaSim compilation scripts
ipdb.export_make(script_path="vsim/vcompile/ips")
ipdb.export_make(script_path="vsim/vcompile/rtl", source='rtl')

# generate vsim.tcl with ModelSim/QuestaSim "linking" script
ipdb.generate_vsim_tcl("vsim/tcl_files/config/vsim_ips.tcl")
ipdb.generate_vsim_tcl("vsim/tcl_files/config/vsim_rtl.tcl", source='rtl')

# generate script to compile all IPs for ModelSim/QuestaSim
ipdb.generate_makefile("vsim/vcompile/ips.mk")
ipdb.generate_makefile("vsim/vcompile/rtl.mk", source='rtl')

print(tcolors.OK + "Generated new scripts for IPs!" + tcolors.ENDC)

