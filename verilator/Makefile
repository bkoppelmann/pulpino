CXX=g++
LIBS=\
 ../ips/zero-riscy/include/zeroriscy_defines.sv \
 ../ips/zero-riscy/include/zeroriscy_config.sv \

RTL=../rtl/pulpino_top.sv
TOP=pulpino_top
SIM=$(TOP)_sim.elf

CFLAGS=-Iobj_dir -I/usr/share/verilator/include -g3 -std=c++11
VERICSRC=/usr/share/verilator/include/verilated_vcd_c.cpp \
		 /usr/share/verilator/include/verilated.cpp
VFLAGS=-sv \
	   --trace \
	   --trace-max-array 8192 \
	   -Wno-fatal \
	   -pvalue+USE_ZERO_RISCY=1 \
	   -pvalue+ZERO_RV32M=1 \
	   -pvalue+ZERO_RV32E=0 \
	   -I../rtl/includes \
	   -I../rtl/components \
	   -I../rtl \
	   -I../ips/apb/apb_node \
	   -I../ips/axi/axi2apb \
	   -I../ips/axi/axi_slice \
	   -I../ips/axi/axi_spi_slave \
	   -I../ips/axi/axi_slice_dc \
	   -I../ips/axi/core2axi \
	   -I../ips/axi/axi_node \
	   -I../ips/axi/axi_mem_if_DP \
	   -I../ips/apb/apb_uart_sv \
	   -I../ips/apb/apb_gpio \
	   -I../ips/apb/apb_spi_master \
	   -I../ips/apb/apb_timer \
	   -I../ips/apb/apb_event_unit/include/ \
	   -I../ips/apb/apb_event_unit \
	   -I../ips/apb/apb_i2c \
	   -I../ips/apb/apb_fll_if \
	   -I../ips/apb/apb_pulpino \
	   -I../ips/apb/apb2per \
	   -I../ips/zero-riscy \
	   -I../ips/zero-riscy/include \
	   -I../ips/adv_dbg_if/rtl \

.PHONY: all clean view run
all: $(SIM)

run: $(SIM)
	./$(SIM)

view:
	gtkwave -a signals/current.gtkw vlt_dump.vcd&

main.o: main.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

obj_dir/V$(TOP)__ALL.a: $(RTL)
	verilator $(LIBS) $(VFLAGS) --top-module $(TOP) -cc $<
	cd obj_dir/ && make -f V$(TOP).mk

$(SIM): obj_dir/V$(TOP)__ALL.a main.o
	$(CXX) $(CFLAGS) main.o $(VERICSRC) obj_dir/Vpulpino_top__ALL.a -o $(SIM)

clean:
	rm -rf obj_dir/
	rm -rf $(SIM)
	rm -rf *.o

