#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


/// Signals for the data interface.
struct Data {
  bool mem_valid;
  bool mem_instr;
  bool mem_ready;
  uint32_t mem_addr;
  uint32_t mem_wdata;
  uint8_t mem_wstrb;
  uint32_t mem_rdata;
};
/// Look-Ahead interface.
struct LookAhead {
  bool mem_la_read;
  bool mem_la_write;
  uint32_t mem_la_addr;
  uint32_t mem_la_wdata;
  uint8_t  mem_la_wstrb;
};
/// Pico Co-Processor Interface (PCPI)  
struct PCPI {
  bool pcpi_valid;
  uint32_t pcpi_insn;
  uint32_t pcpi_rs1;
  uint32_t pcpi_rs2;
  bool pcpi_wr;
  uint32_t pcpi_rd;
  bool pcpi_wait;
  bool pcpi_ready;
};
/// IRQ Interface
struct IRQ {
  uint32_t irq;
  uint32_t eoi;
};
/// Trace Interface
struct Trace {
  bool trace_valid;
  uint64_t trace_data;
};

/// Abstract interface to an Arcilator or Verilator model.
class Picorv32Model {
public:
  static constexpr const char *PORT_NAMES[] = {
#define PORT(name) #name,
#include "ports.def"
  };
  static constexpr size_t NUM_PORTS = sizeof(PORT_NAMES) / sizeof(*PORT_NAMES);
  using Ports = std::array<uint64_t, NUM_PORTS>;

  Picorv32Model() {}
  virtual ~Picorv32Model();

  virtual void vcd_start(const char *outputFile) = 0;
  virtual void vcd_dump(size_t cycle) = 0;
  virtual void eval(bool advance_clock = false) = 0;
  virtual Ports get_ports() { return {}; }
  virtual void set_clock(bool clock) = 0;
  virtual void set_reset(bool resetn) = 0;
  virtual void get_trap (bool trap) = 0;
  virtual void set_Data(const Data &in) = 0;
  virtual void get_Data(Data &out) = 0;
  virtual void get_LookAhead(LookAhead &out) = 0;
  virtual void set_PCPI(const PCPI &in) = 0;
  virtual void get_PCPI(PCPI &out) = 0;
  virtual void set_IRQ(const IRQ &in) = 0;
  virtual void get_IRQ(IRQ &out) = 0;
  virtual void set_Trace(const Trace &in) = 0;
  virtual void get_Trace(Trace &out) = 0;

  const char *name = "unknown";
  std::chrono::high_resolution_clock::duration duration =
      std::chrono::high_resolution_clock::duration::zero();
  std::chrono::high_resolution_clock::duration clock_time =
      std::chrono::high_resolution_clock::duration::zero();
  std::chrono::high_resolution_clock::duration passthrough_time =
      std::chrono::high_resolution_clock::duration::zero();
};

std::unique_ptr<Picorv32Model> makeArcilatorModel();
std::unique_ptr<Picorv32Model> makeVerilatorModel();
std::unique_ptr<Picorv32Model> makeVerilatorCirctModel();
