/*
 *  Copyright (c) 2019 Bastian Koppelmann, Paderborn Univeristy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <getopt.h>
#include <chrono>
#include <ctime>
#include <signal.h>
#include <unistd.h>
#include <sstream>
#include <fstream>

#include "Vpulpino_top.h"
#include "verilated_vcd_c.h"
#include "verilated.h"

#define CLK_DELAY 40 // ns
#define SCLK_DELAY 50 // ns
#define JCLK_DELAY 40 // ns

static vluint64_t main_time = 0;

double sc_time_stamp () {
    return main_time;
}

/*
 * Hacky UART receiver:
 *     it is coupled to 40ns clk period as I don't know yet how to sample
 *     uart_tx asynchronously.
 */

uint32_t last_tx=1;
bool uart_falling_edge(uint32_t tx) {
    if (last_tx == 1 && tx == 0) {
        last_tx = 0;
        return true;
    }
    return false;
}

void uart_rising_edge(uint32_t tx) {
    if (last_tx == 0 && tx == 1) {
        last_tx = 1;
    }
}

bool start_reading = false;
uint32_t uart_bit_idx=0;
int64_t ticks=0;
uint8_t uart_byte=0;

void uart_recv_bit(Vpulpino_top *top)
{
    if (ticks > 3) {
        uart_byte |= (top->uart_tx << uart_bit_idx);
        uart_bit_idx++;
        ticks=0;
    }
}

void uart_print_byte(uint8_t byte) {
    printf("%c", byte);
    start_reading = false;
}

void update_uart(Vpulpino_top *top, VerilatedVcdC *tfp)
{
    if (uart_falling_edge(top->uart_tx)) {
        /* HACK: The start bit is active for 3 clock periods
         * the other bits are active for 2 clock periods.
         */
        ticks = -3;
        uart_byte=0;
        uart_bit_idx = 0;
        start_reading = true;
    }

    if (start_reading) {
        uart_recv_bit(top);
        if (uart_bit_idx > 7) {
            uart_print_byte(uart_byte);
        }
        ticks++;
    } else {
        uart_rising_edge(top->uart_tx);
    }
}

void run_tick_clk(Vpulpino_top *tb, VerilatedVcdC *tfp)
{
    tb->clk = 1;
    tb->eval();
    main_time += CLK_DELAY /2;
    update_uart(tb, tfp);
#ifdef VM_TRACE
    tfp->dump(static_cast<vluint64_t>(main_time));
#endif
    tb->clk = 0;
    tb->eval();
    main_time += CLK_DELAY /2;
    update_uart(tb, tfp);
#ifdef VM_TRACE
    tfp->dump(static_cast<vluint64_t>(main_time));
#endif
}


void preload_hex(Vpulpino_top *top, VerilatedVcdC *tfp, const char *filepath)
{
    std::ifstream inputFileStream(filepath);
	std::string line;
	std::map<uint32_t,uint32_t> mymap;
	while(std::getline(inputFileStream, line)){
		std::istringstream lineStream(line);
		uint32_t addr,data;
		char dummy;
		lineStream >> std::hex >> addr >> dummy >> data;
		mymap.insert ( std::pair<uint32_t,uint32_t>(addr,data) );
	}

	uint32_t mem_start=0;
	printf("Preloading Instruction RAM\n");
	std::map<uint32_t,uint32_t>::iterator it = mymap.begin();
	for (it=mymap.begin(); it!=mymap.end(); ++it){
		if (it->first != (std::prev(it)->first + 4)) {
			printf("prev address %x current addr %x\n", std::prev(it)->first, it->first);
			printf("Preloading Data RAM\n");
			mem_start = it->first;
		}
		if (mem_start == 0) {
			top->pulpino_top__DOT__core_region_i__DOT__instr_mem__DOT__sp_ram_wrap_i__DOT__sp_ram_i__DOT__mem[it->first/4] = it->second;
		} else {
			top->pulpino_top__DOT__core_region_i__DOT__data_mem__DOT__sp_ram_i__DOT__mem[(it->first - mem_start)/4] = it->second;
		}
	}
    inputFileStream.close();
}

void raise_gpio(Vpulpino_top *top, VerilatedVcdC *tfp)
{
    top->gpio_in |= 1 << 16;
    run_tick_clk(top, tfp);
    top->gpio_in &= ~(1 << 16);
}

/*static int raise_interrupt = 0;
void sig_user_handler(int sig)
{
    if (sig == SIGUSR1) {
        raise_interrupt = 1;
    }
}*/

void run_simulation(Vpulpino_top *top, VerilatedVcdC *tfp)
{
    top->fetch_enable_i = 1;
    do {
        run_tick_clk(top, tfp);
        /*/if (raise_interrupt == 1) {
            raise_gpio(top, tfp);
            raise_interrupt = 0;
        }*/
    } while ((top->gpio_out & (1 << 8)) == 0);
}

void read_user_input()
{
    while (1) {
        getchar();
        //kill(0, SIGUSR1);
    }
}


void reset(Vpulpino_top *top, VerilatedVcdC *tfp)
{
    top->testmode_i = 0;
    top->clk_sel_i = 0;
    top->spi_cs_i = 1;
    top->rst_n = 1;
    top->scan_enable_i = 0;
    top->fetch_enable_i = 0;
    top->scl_pad_i = 0;
    top->eval();
#ifdef VM_TRACE
    tfp->dump(static_cast<vluint64_t>(main_time));
#endif
    main_time += 10;
    top->rst_n = 0;
    top->eval();
#ifdef VM_TRACE
    tfp->dump(static_cast<vluint64_t>(main_time));
#endif

    for(int i = 0; i < 100; i++) {
        run_tick_clk(top, tfp);
    }
    top->rst_n = 1;
    top->eval();
#ifdef VM_TRACE
    tfp->dump(static_cast<vluint64_t>(main_time));
#endif
}

int main(int argc, char **argv) {

#ifdef VM_TRACE
    FILE *vcdfile = NULL;
#endif

    // Initialize Verilators variables
    Verilated::commandArgs(argc, argv);

    // Create an instance of our module under test
    Vpulpino_top *top = new Vpulpino_top;
    VerilatedVcdC* tfp = NULL;
#ifdef VM_TRACE
    Verilated::traceEverOn(true);  // Verilator must compute traced signals
    VL_PRINTF("Enabling waves into logs/vlt_dump.vcd...\n");
    tfp = new VerilatedVcdC;
    top->trace(tfp, 99);  // Trace 99 levels of hierarchy
    tfp->open("vlt_dump.vcd");  // Open the dump file
#endif

    reset(top, tfp);
    preload_hex(top, tfp, "sw/helloworld.hex");

    //signal(SIGUSR1, sig_user_handler);
    //pid_t pid = fork();
    /*if (pid == 0) {
        read_user_input();
    } else {*/
        run_simulation(top, tfp);
    //}
#ifdef VM_TRACE
    if (tfp)
        tfp->close();
    if (vcdfile)
        fclose(vcdfile);
#endif

    exit(EXIT_SUCCESS);
}
