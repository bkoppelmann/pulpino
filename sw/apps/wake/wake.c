// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.


#include <stdio.h>
#include "gpio.h"
#include "int.h"
#include "utils.h"

void ISR_GPIO(void)
{
    get_gpio_irq_status();
    ICP = 0xffffffff; // Clear all interrupts
    EER = 0x1; // Only Events wake up the CPU properly
    ESP = 0x1;
    printf("Die Temperatur ist 5C\n");
}

int main()
{
  set_gpio_pin_direction(16, DIR_IN);
  set_pin_function(16, FUNC_GPIO);
  set_gpio_pin_irq_en(16, 1);
  set_gpio_pin_irq_type(16, GPIO_IRQ_RISE);
  IER = 0x1 << GPIO_EVENT;
  int_enable();
  printf("Waiting for Interrupts ... \n");
  while (1) {
    sleep();
  }
  return 0;
}
