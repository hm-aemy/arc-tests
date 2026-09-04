#include "picorv32-arc.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

// 32bit unit to 4x 8bit conversion union
union conversion_union{
   uint32_t value_32;
   uint8_t values_8[4];
};


// A 10ns clock period. The runtime keeps time in femtoseconds.
static constexpr uint64_t halfPeriodFs = 5'000'000;

class Testbench {
public:
  // From "picorv32-arc.h"
  picorv32 model;

  // `args` is the runtime option string, e.g. "vcd;traceFile=picorv32.vcd".
  explicit Testbench(const char *args) : model(args) { set_time(); }

  void tick() {
    model.view.clk = 0;
    step();
    model.view.clk = 1;
    step();
  }

private:
  void step() {
    time += halfPeriodFs;
    set_time();
    model.eval();
  }

  // The runtime reads the simulation time as an i64 from offset 0 of the model
  // state; it timestamps trace samples with it.
  void set_time() { *reinterpret_cast<uint64_t *>(model.storage) = time; }

  uint64_t time = 0;
};

int main(int argc, char **argv) {
  std::string runtimeArgs;

  // Commands in Memory
  uint8_t memory [256][4] = {
    {0x3f, 0xc0, 0x00, 0x93}, //       li      x1,1020 
    {0x00, 0x00, 0xa0, 0x23}, //       sw      x0,0(x1)
    {0x00, 0x00, 0xa1, 0x03}, // loop: lw      x2,0(x1)
    {0x00, 0x11, 0x01, 0x13}, //       addi    x2,x2,1
    {0x00, 0x20, 0xa0, 0x23}, //       sw      x2,0(x1)
    {0xff, 0x5f, 0xf0, 0x6f}  //       j       <loop>
  };

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--trace") == 0 && i + 1 < argc) {
      // `vcd` selects the encoder, `traceFile` overrides the default name.
      runtimeArgs += "vcd;traceFile=" + std::string(argv[++i]) + ";";
    } else {
      fprintf(stderr, "usage: %s [--trace <file.vcd>]\n", argv[0]);
      return 1;
    }
  }

  Testbench tb(runtimeArgs.empty() ? nullptr : runtimeArgs.c_str());
  auto &view = tb.model.view;

  view.resetn = 0;
  tb.tick();
  tb.tick();
  view.resetn = 1;
  for (unsigned i = 1; i <= 64; ++i) {
    tb.tick();

    if (view.mem_valid && view.mem_ready) {
      if (view.mem_instr){
        printf("ifetch 0x%08x: 0x%08x /n", view.mem_addr, view.mem_rdata);
      } else if (view.mem_wstrb){
        printf("write  0x%08x: 0x%08x (wstrb=%b) /n", view.mem_addr, view.mem_wdata, view.mem_wstrb);
      } else {
        printf("read   0x%08x: 0x%08x /n", view.mem_addr, view.mem_rdata);
      }
    }

    conversion_union read_data;
    conversion_union write_data;
    conversion_union write_strob;
    view.mem_ready = 0;
    if (view.mem_valid && !view.mem_ready) {
      if (view.mem_addr < 1024) {
        view.mem_ready = 1;

        read_data.values_8[0] = memory[view.mem_addr >> 2][0];
        read_data.values_8[1] = memory[view.mem_addr >> 2][1];
        read_data.values_8[2] = memory[view.mem_addr >> 2][2];
        read_data.values_8[3] = memory[view.mem_addr >> 2][3];
        view.mem_rdata = read_data.value_32;

        write_data.value_32 = view.mem_wdata;
        write_strob.value_32 = view.mem_wstrb;
        if (write_strob.values_8[0]) memory[view.mem_addr >> 2][0] = write_data.values_8[0];
				if (write_strob.values_8[1]) memory[view.mem_addr >> 2][1] = write_data.values_8[1];
				if (write_strob.values_8[2]) memory[view.mem_addr >> 2][2] = write_data.values_8[2];
				if (write_strob.values_8[3]) memory[view.mem_addr >> 2][3] = write_data.values_8[3];
      }
    }
  }
}
