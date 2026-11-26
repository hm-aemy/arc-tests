#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


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
  virtual void set_clk(bool clk) = 0;
  virtual void set_resetn(bool resetn) = 0;
  virtual void get_trap (bool trap) = 0;
  virtual void get_trace_valid (bool trace_valid) = 0;
  virtual void get_trace_data(uint64_t trace_data) = 0;

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
