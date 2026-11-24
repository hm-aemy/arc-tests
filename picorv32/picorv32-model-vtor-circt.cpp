#include "picorv32-model.h"
#include "picorv32-vtor-circt.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorPicorv32Model : public Picorv32Model {
  Vpicorv32Circt model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorPicorv32Model() { name = "vtor-circt"; }
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

void set_clock(bool clk) override { model.clk = clk; }

  void set_reset(bool resetn) override { model.resetn = resetn; }

  void get_trap(bool trap) override {trap = model.trap; }

  void set_Data(const Data &in) override {
    model.mem_ready = in.mem_ready;
    model.mem_rdata = in.mem_rdata;
  }

  void get_Data(Data &out) override {
    out.mem_valid = model.mem_valid;
    out.mem_instr = model.mem_instr;
    out.mem_wdata = model.mem_wdata;
    out.mem_addr = model.mem_addr;
    out.mem_wdata = model.mem_wstrb;
  }

  void get_LookAhead(LookAhead &out) override {
    out.mem_la_read = model.mem_la_read;
    out.mem_la_write = model.mem_la_write;
    out.mem_la_addr = model.mem_la_addr;
    out.mem_la_wdata = model.mem_la_wdata;
    out.mem_la_wstrb = model.mem_la_wstrb;
  }

  void set_PCPI(const PCPI &in) override {
    model.pcpi_wr = in.pcpi_wr;
    model.pcpi_rd = in.pcpi_rd;
    model.pcpi_wait = in.pcpi_wait;
    model.pcpi_ready = in.pcpi_ready;
  }

  void get_PCPI(PCPI &out) /*override*/ {
    out.pcpi_valid = model.pcpi_valid;
    out.pcpi_insn = model.pcpi_insn;
    out.pcpi_rs1 = model.pcpi_rs1;
    out.pcpi_rs2 = model.pcpi_rs2;
  }

  void set_IRQ(const IRQ &in) /*override*/ {
    model.irq = in.irq;
  }

  void get_IRQ(IRQ &out) /*override*/ {
    out.eoi = model.eoi;
  }

  void set_Trace (const Trace &in) /*override*/ {
    model.trace_valid = in.trace_valid;
  }

  void get_Trace (Trace &out) /*override*/ {
    out.trace_data = model.trace_data;
  }
};
} // namespace

std::unique_ptr<Picorv32Model> makeVerilatorCirctModel() {
  return std::make_unique<VerilatorPicorv32Model>();
}
