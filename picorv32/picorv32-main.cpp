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
#ifdef UseFirmwareFile
  uint32_t mem_size = 128*1024/4;
  uint32_t memory [128*1024/4];
#else
  uint32_t mem_size = 256;
  uint32_t memory [256] = {
    {0x3fc00093}, //       li      x1,1020 
    {0x0000a023}, //       sw      x0,0(x1)
    {0x0000a103}, // loop: lw      x2,0(x1)
    {0x00110113}, //       addi    x2,x2,1
    {0x0020a023}, //       sw      x2,0(x1)
    {0xff5ff06f}  //       j       <loop>
  };
#endif

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
  for (unsigned i = 1; i <= 1000; ++i) {
    tb.tick();
  }
  view.resetn = 1;

  for (unsigned i = 1; i <= 1000; ++i) {
    tb.tick();

    conversion_union write_data;
    conversion_union write_strob;
    conversion_union memory_write;
    view.mem_ready = 0;
    if (view.mem_valid && !view.mem_ready) {
      if (view.mem_addr < mem_size*4) {
        view.mem_ready = 1;

        view.mem_rdata = memory[view.mem_addr >> 2];

        write_data.value_32 = view.mem_wdata;
        write_strob.value_32 = view.mem_wstrb;
        memory_write.value_32 = memory[view.mem_addr >> 2];
        if (write_strob.values_8[0]) memory_write.values_8[0] = write_data.values_8[0];
				if (write_strob.values_8[1]) memory_write.values_8[1] = write_data.values_8[1];
				if (write_strob.values_8[2]) memory_write.values_8[2] = write_data.values_8[2];
				if (write_strob.values_8[3]) memory_write.values_8[3] = write_data.values_8[3];
        memory[view.mem_addr >> 2] = memory_write.value_32;
      }

      if (view.mem_instr){
        printf("ifetch 0x%08x: 0x%08x \n", view.mem_addr, view.mem_rdata);
      } else if (view.mem_wstrb){
        printf("write  0x%08x: 0x%08x (wstrb=%b) \n", view.mem_addr, view.mem_wdata, view.mem_wstrb);
      } else {
        printf("read   0x%08x: 0x%08x \n", view.mem_addr, view.mem_rdata);
      }
    }
  }
}
