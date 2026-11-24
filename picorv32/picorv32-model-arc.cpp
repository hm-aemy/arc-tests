#include "picorv32-model.h"
#include "build/picorv32-arc.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorPicorv32Model : public Picorv32Model {
  picorv32_th model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<picorv32_thLayout>> model_vcd;
  bool is_first_vcd_dump = true;

public:
  ArcilatorPicorv32Model()
#ifdef ARC_USE_COMPILED_RUNTIME_LIB
      : model(picorv32_th("debug"))
#endif
  {
    name = "arcs";
  }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    // model_vcd.reset(
    //     new ValueChangeDump<Picorv32_thLayout>(model.vcd(vcd_stream)));
    model_vcd.reset(
        new ValueChangeDump<picorv32_thLayout>(vcd_stream, &model.storage[0]));
    model_vcd->writeHeader();
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      vcd_stream << "#" << cycle << "\n";
      model_vcd->writeValues(is_first_vcd_dump);
      is_first_vcd_dump = false;
    }
  }

  void eval(bool advance_clock) override { model.eval(); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  void set_clock(bool clk) override { model.view.clk = clk; }

  void set_reset(bool resetn) override { model.view.resetn = resetn; }

  void get_trap(bool trap) override {trap = model.view.trap; }

  void set_Data(const Data &in) override {
    model.view.mem_ready = in.mem_ready;
    model.view.mem_rdata = in.mem_rdata;
  }

  void get_Data(Data &out) override {
    out.mem_valid = model.view.mem_valid;
    out.mem_instr = model.view.mem_instr;
    out.mem_wdata = model.view.mem_wdata;
    out.mem_addr = model.view.mem_addr;
    out.mem_wdata = model.view.mem_wstrb;
  }

  void get_LookAhead(LookAhead &out) override {
    out.mem_la_read = model.view.mem_la_read;
    out.mem_la_write = model.view.mem_la_write;
    out.mem_la_addr = model.view.mem_la_addr;
    out.mem_la_wdata = model.view.mem_la_wdata;
    out.mem_la_wstrb = model.view.mem_la_wstrb;
  }

  void set_PCPI(const PCPI &in) override {
    model.view.pcpi_wr = in.pcpi_wr;
    model.view.pcpi_rd = in.pcpi_rd;
    model.view.pcpi_wait = in.pcpi_wait;
    model.view.pcpi_ready = in.pcpi_ready;
  }

  void get_PCPI(PCPI &out) override {
    out.pcpi_valid = model.view.pcpi_valid;
    out.pcpi_insn = model.view.pcpi_insn;
    out.pcpi_rs1 = model.view.pcpi_rs1;
    out.pcpi_rs2 = model.view.pcpi_rs2;
  }

  void set_IRQ(const IRQ &in) override {
    model.view.irq = in.irq;
  }

  void get_IRQ(IRQ &out) override {
    out.eoi = model.view.eoi;
  }

  void set_Trace (const Trace &in) override {
    model.view.trace_valid = in.trace_valid;
  }

  void get_Trace (Trace &out) override {
    out.trace_data = model.view.trace_data;
  }
};
} // namespace

std::unique_ptr<Picorv32Model> makeArcilatorModel() {
  return std::make_unique<ArcilatorPicorv32Model>();
}
