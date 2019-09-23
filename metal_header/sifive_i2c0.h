/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __METAL_HEADER_SIFIVE_I2C0__H
#define __METAL_HEADER_SIFIVE_I2C0__H

#include <device.h>
#include <regex>

class sifive_i2c0 : public Device {
public:
  int num_i2cs;
  uint32_t max_interrupts = 0;

  sifive_i2c0(std::ostream &os, const fdt &dtb);
  void create_defines();
  void include_headers();
  void declare_inlines();
  void define_inlines();
  void declare_structs();
  void define_structs();
  void create_handles();
};

#endif
