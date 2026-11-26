#include "picorv32-model.h"
#include "picorv32-vtor.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorPicorv32Model : public Picorv32Model {
  Vpicorv32 model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorPicorv32Model() { name = "vtor"; }
  ~VerilatorPicorv32Model() {
    if (model_vcd)
      model_vcd->close();
  }

  void vcd_start(const char *outputFile) override {
    Verilated::traceEverOn(true);
    model_vcd = std::make_unique<VerilatedVcdC>();
#ifdef TRACE
    model.trace(model_vcd.get(), 10000);
#endif
    model_vcd->open(outputFile);
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd)
      model_vcd->dump(static_cast<uint64_t>(cycle));
  }

  void eval(bool advance_clock) override { model.eval(); }

  Ports get_ports() override {
    return {
#define PORT(name) model.name,
#include "ports.def"
    };
  }

  void set_clk(bool clk) override { model.view.clk = clk; }

  void set_resetn(bool resetn) override { model.view.resetn = resetn; }

  void get_trap(bool trap) override {trap = model.view.trap; }

  void get_trace_valid(bool trace_valid) override {
    out.trace_valid = model.view.trace_valid;
  }

  void get_trace_data(uint64_t trace_data) override {
    out.trace_data = model.view.trace_data;
  }
};
} // namespace

std::unique_ptr<Picorv32Model> makeVerilatorModel() {
  return std::make_unique<VerilatorPicorv32Model>();
}
