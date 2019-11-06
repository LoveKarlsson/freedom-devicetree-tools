/* Copyright (c) 2019 SiFive Inc. */
/* SPDX-License-Identifier: Apache-2.0 */

#include "chosen_strategy.h"
#include <ranges.h>
#include <regs.h>

#include <layouts/default_layout.h>
#include <layouts/scratchpad_layout.h>
#include <layouts/ramrodata_layout.h>

bool ChosenStrategy::valid(const fdt &dtb, list<Memory> available_memories)
{
  bool chosen_ram = false;
  bool chosen_rom = false;

  dtb.chosen(
    "metal,ram",
    tuple_t<node, uint32_t>(),
    [&](node n, uint32_t idx) {
      chosen_ram = true;
    });
  dtb.chosen(
    "metal,rom",
    tuple_t<node, uint32_t>(),
    [&](node n, uint32_t idx) {
      chosen_rom = true;
    });

  return (chosen_ram && chosen_rom);
}

LinkerScript ChosenStrategy::create_layout(const fdt &dtb, list<Memory> available_memories,
                                             LinkStrategy link_strategy)
{
  Memory rom_memory;
  Memory ram_memory;
  Memory itim_memory;
  bool has_itim = false;

  auto extract_node_props = [](Memory &m, const node &n, uint32_t idx) {
    if (n.field_exists("reg-names")) {
      n.named_tuples(
        "reg-names",
        "reg", "mem",
        tuple_t<target_addr, target_size>(),
        [&](target_addr b, target_size s) {
          m.base = b;
          m.size = s;
        });
    } else if (n.field_exists("ranges")) {
      ranges_t ranges = get_ranges(n);

      /* TODO: How do we pick which of the ranges entries to use? */
      if (!ranges.empty()) {
        auto it = std::next(ranges.begin(), idx);
        m.base = (*it).child_address;
        m.size = (*it).size;
      }
    } else if (n.field_exists("reg")) {
      regs_t regs = get_regs(n);

      /* TODO: How do we pick which of the regs entries to use? */
      if (!regs.empty()) {
        auto it = std::next(regs.begin(), idx);
        m.base = (*it).address;
        m.size = (*it).size;
      }
    }  else {
      n.maybe_tuple(
        "reg",
        tuple_t<target_addr, target_size>(),
        [&]() {},
        [&](target_addr b, target_size s) {
          m.base = b;
          m.size = s;
        });
    }

    n.maybe_tuple(
        "compatible",
        tuple_t<string>(),
        [&]() {},
        [&](string compat) {
          m.compatible = compat;
        });
  };

  dtb.chosen(
    "metal,ram",
    tuple_t<node, uint32_t>(),
    [&](node n, uint32_t idx) {
      ram_memory.name = "ram";
      extract_node_props(ram_memory, n, idx);
      ram_memory.attributes = "wxa!ri";
    });
  dtb.chosen(
    "metal,rom",
    tuple_t<node, uint32_t>(),
    [&](node n, uint32_t idx) {
      rom_memory.name = "flash";
      extract_node_props(rom_memory, n, idx);
      rom_memory.attributes = "rxai!w";
    });
  dtb.chosen(
    "metal,itim",
    tuple_t<node>(),
    [&](node n) {
      itim_memory.name = "itim";
      extract_node_props(itim_memory, n, 0);
      itim_memory.attributes = "wx!rai";

      has_itim = true;
    });

  /* Generate the layouts */

  if (has_itim) {
    print_chosen_strategy("ChosenStrategy", link_strategy, ram_memory, rom_memory, itim_memory);
  } else {
    print_chosen_strategy("ChosenStrategy", link_strategy, ram_memory, rom_memory, ram_memory);
  }

  switch (link_strategy) {
  default:
  case LINK_STRATEGY_DEFAULT:
    if (has_itim) {
      return DefaultLayout(dtb, rom_memory, itim_memory, ram_memory, ram_memory);
    } else {
      return DefaultLayout(dtb, rom_memory, rom_memory, ram_memory, ram_memory);
    }
    break;

  case LINK_STRATEGY_SCRATCHPAD:
    if (has_itim) {
      return ScratchpadLayout(dtb, ram_memory, itim_memory, ram_memory, ram_memory);
    } else {
      return ScratchpadLayout(dtb, ram_memory, ram_memory, ram_memory, ram_memory);
    }
    break;

  case LINK_STRATEGY_RAMRODATA:
    if (has_itim) {
      return RamrodataLayout(dtb, rom_memory, itim_memory, ram_memory, rom_memory);
    } else {
      return RamrodataLayout(dtb, rom_memory, rom_memory, ram_memory, rom_memory);
    }
    break;
  }
}

