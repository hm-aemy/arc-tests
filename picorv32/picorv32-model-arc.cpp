#include "picorv32-model.h"
#include "build/picorv32-arc.h"
#include <fstream>
#include <optional>
/*
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

  void set_clk(bool clk) override { model.view.clk = clk; }

  void set_reset(bool resetn) override { model.view.resetn = resetn; }

  void get_trap(bool trap) override {trap = model.view.trap; }

  void get_trace_valid(bool trace_valid) override {
    out.trace_valid = model.view.trace_valid;
  }

  void get_trace_data(uint64_t trace_data) override {
    out.trace_data = model.view.trace_data;
  }
};
} // namespace

std::unique_ptr<Picorv32Model> makeArcilatorModel() {
  return std::make_unique<ArcilatorPicorv32Model>();
}
*/