#include <cstdint>

#include "elfio/elfio.hpp"
#include "picorv32-model.h"
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>

#define TOHOST_ADDR 0x60000000
#define FROMHOST_ADDR 0x60000040
#define TOHOST_DATA_ADDR 0x60000080
#define TOHOST_DATA_SIZE 64 // bytes
#define SYS_write 64
#define BOOTROM_BASE 0x10000

double sc_time_stamp() { return 0; }

static bool finished = false;

Picorv32Model::~Picorv32Model() {}

class ComparingPicorv32Model : public Picorv32Model {
public:
  std::vector<std::unique_ptr<Picorv32Model>> models;
  size_t cycle = 0;
  size_t num_mismatches = 0;

  virtual ~ComparingPicorv32Model() {
    std::cerr << "----------------------------------------\n";
    std::cerr << cycle << " cycles total\n";
    for (auto &model : models) {
      auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                         model->duration)
                         .count();
      std::cerr << model->name << ": " << (cycle / seconds) << " Hz\n";
      seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                    model->clock_time)
                    .count();
      std::cerr << model->name << ": time in clock eval " << seconds << " sec ("
                << (cycle / seconds) << " Hz)\n";
      seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                    model->passthrough_time)
                    .count();
      std::cerr << model->name << ": time in passthrough eval " << seconds
                << " sec (" << (cycle / seconds) << " Hz)\n";
    }
  }

  void vcd_start(const char *outputFile) override {
    for (auto &model : models) {
      std::string extendedFile{outputFile};
      auto pos = extendedFile.rfind('.');
      extendedFile.insert(pos, model->name);
      extendedFile.insert(pos, "-");
      model->vcd_start(extendedFile.data());
    }
  }

  void vcd_dump(size_t cycle) override {
    for (auto &model : models)
      model->vcd_dump(cycle);
  }

  void clock() {
    eval();
    for (unsigned i = 0; i < 2; ++i) {
      compare_ports();
      vcd_dump(cycle);
      ++cycle;
      set_clock(i == 1);
      eval(i == 1);
    }
  }

  void eval(bool advance_clock = false) override {
    for (auto &model : models) {
      auto t_before = std::chrono::high_resolution_clock::now();
      model->eval();
      auto t_after = std::chrono::high_resolution_clock::now();
      model->duration += t_after - t_before;
      if (advance_clock)
        model->clock_time += t_after - t_before;
      else
        model->passthrough_time += t_after - t_before;
    }
  }
  

  void set_clock(bool clock) override {
    for (auto &model : models)
      model->set_clock(clock);
  }

  void set_reset(bool reset) override {
    for (auto &model : models)
      model->set_reset(reset);
  }

  void set_Data(const Data &in) override {
    for (auto &model : models)
      model->set_Data(in);
  }

  void get_trap(bool trap) override {
    if (!models.empty())
      models[0]->get_trap(trap);
  }

  void get_Data(Data &out) override {
    if (!models.empty())
      models[0]->get_Data(out);
  }

  void get_LookAhead(LookAhead &out) override {
    if (!models.empty())
      models[0]->get_LookAhead(out);
  }

  void set_PCPI(const PCPI &in) override {
    for (auto &model : models)
      model->set_PCPI(in);
  }

  void get_PCPI(PCPI &out) override {
    if (!models.empty())
      models[0]->get_PCPI(out);
  }

  void set_IRQ(const IRQ &in) override {
    for (auto &model : models)
      model->set_IRQ(in);
  }

  void get_IRQ(IRQ &out) override {
    if (!models.empty())
      models[0]->get_IRQ(out);
  }

  void set_Trace(const Trace &in) override {
    for (auto &model : models)
      model->set_Trace(in);
  }

  void get_Trace(Trace &out) override {
    if (!models.empty())
      models[0]->get_Trace(out);
  }



  void compare_ports() {
    if (models.size() < 2)
      return;
    auto portsA = models[0]->get_ports();
    for (unsigned modelIdx = 1; modelIdx < models.size(); ++modelIdx) {
      auto portsB = models[modelIdx]->get_ports();
      for (unsigned portIdx = 0; portIdx < portsA.size(); ++portIdx) {
        if (portsA[portIdx] == portsB[portIdx])
          continue;
        ++num_mismatches;
        std::cerr << "cycle " << cycle << ": mismatching " << std::hex
                  << PORT_NAMES[portIdx] << ": " << portsA[portIdx] << " ("
                  << models[0]->name << ") != " << portsB[portIdx] << " ("
                  << models[modelIdx]->name << ")\n"
                  << std::dec;
      }
    }
  }
};

int main(int argc, char **argv) {
  //===--------------------------------------------------------------------===//
  // Process CLI arguments
  //===--------------------------------------------------------------------===//

  bool optRunAll = true;
  bool optRunArcs = false;
  bool optRunVtor = false;
  bool optRunVtorCirct = false;
  char *optVcdOutputFile = nullptr;

  char **argOut = argv + 1;
  for (char **arg = argv + 1, **argEnd = argv + argc; arg != argEnd; ++arg) {
    if (strcmp(*arg, "--arcs") == 0) {
      optRunAll = false;
      optRunArcs = true;
      continue;
    }
    if (strcmp(*arg, "--vtor") == 0) {
      optRunAll = false;
      optRunVtor = true;
      continue;
    }
    if (strcmp(*arg, "--vtor-circt") == 0) {
      optRunAll = false;
      optRunVtorCirct = true;
      continue;
    }
    if (strcmp(*arg, "--trace") == 0) {
      ++arg;
      if (arg == argEnd) {
        std::cerr << "missing trace output file name after `--trace`\n";
        return 1;
      }
      optVcdOutputFile = *arg;
      continue;
    }
    *argOut++ = *arg;
  }
  argc = argOut - argv;

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " [options] <binary>\n";
    std::cerr << "options:\n";
    std::cerr << "  --arcs         run arcilator simulation\n";
    std::cerr << "  --vtor         run verilator simulation\n";
    std::cerr << "  --vtor-circt   run verilator round-trip simulation\n";
    std::cerr << "  --trace <VCD>  write trace to <VCD> file\n";
    return 1;
  }

  //===--------------------------------------------------------------------===//
  // Read ELF into memory
  //===--------------------------------------------------------------------===//

  std::map<uint32_t, uint64_t> memory;
  {
    ELFIO::elfio elf;
    if (!elf.load(argv[1])) {
      std::cerr << "unable to open file " << argv[1] << std::endl;
      return 1;
    }
    std::cerr << std::hex;
    for (const auto &segment : elf.segments) {
      if (segment->get_type() != ELFIO::PT_LOAD ||
          segment->get_memory_size() == 0)
        continue;
      std::cerr << "loading segment at " << segment->get_physical_address()
                << " (virtual address " << segment->get_virtual_address()
                << ")\n";
      for (unsigned i = 0; i < segment->get_memory_size(); ++i) {
        uint32_t addr = segment->get_physical_address() + i;
        uint8_t data = 0;
        if (i < segment->get_file_size())
          data = segment->get_data()[i];
        constexpr unsigned bpw = sizeof(memory[0]);
        auto &slot = memory[addr / bpw * bpw];
        slot &= ~((uint64_t)0xFF << ((addr % bpw) * 8));
        slot |= (uint64_t)data << ((addr % bpw) * 8);
      }
    }
    std::cerr << "entry " << elf.get_entry() << "\n";
    std::cerr << std::dec;
    std::cerr << "loaded " << memory.size() * sizeof(memory[0])
              << " program bytes\n";
  }

  // Place bootrom.
  memory[BOOTROM_BASE + 0] = 0;
  memory[BOOTROM_BASE + 0] |= 0x80000537UL;       // lui a0, 0x80000
  memory[BOOTROM_BASE + 0] |= 0x00050067UL << 32; // jr a0  # 0x80000000

  // Allocate the simulation models.
  ComparingPicorv32Model model;
  if (optRunAll || optRunVtor)
    model.models.push_back(makeVerilatorModel());
  if (optRunAll || optRunVtorCirct)
    model.models.push_back(makeVerilatorCirctModel());
  if (optRunAll || optRunArcs)
    model.models.push_back(makeArcilatorModel());
  if (optVcdOutputFile)
    model.vcd_start(optVcdOutputFile);

  //===--------------------------------------------------------------------===//
  // Model initialization and reset
  //===--------------------------------------------------------------------===//

  for (unsigned i = 0; i < 1000; ++i) {
    model.set_reset(i < 100);
    model.clock();
  }

  return 0;
}
