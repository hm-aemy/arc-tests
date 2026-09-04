# Arcilator Makefile for SystemVerilog picorv32
#
#   circt-verilog -> core dialect IR
#   arcilator -> LLVM IR
#   cxx -> executable
#
# Dependencies:
# - uv

MODEL     ?= picorv32
TOP       ?= picorv32
TRACE     ?= 0
BUILD_DIR ?= build

ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
CIRCT_INCLUDE_DIR    ?= $(ARCILATOR_UTILS_ROOT)/../../include
CIRCT_LIB_DIR        ?= $(ARCILATOR_UTILS_ROOT)/../lib

PYTHON ?= uv run --quiet --with jinja2 python3

CXXFLAGS ?= -O2 -Wall -std=c++17
CXXFLAGS += -I$(ARCILATOR_UTILS_ROOT) -I$(BUILD_DIR)
CXXFLAGS += -DARC_USE_COMPILED_RUNTIME_LIB -I$(CIRCT_INCLUDE_DIR)
LDFLAGS  ?=
LDFLAGS  += -L$(CIRCT_LIB_DIR) -l CIRCTArcRuntime -Wl,-rpath,$(CIRCT_LIB_DIR)

ifeq ($(TRACE),1)
  # --trace-taps adds the instrumentation that feeds observed values to the
  # runtime; the --observe-* flags decide which values those are.
  ARCILATOR_ARGS += --trace-taps
  ARCILATOR_ARGS += --observe-wires 
  ARCILATOR_ARGS += --observe-ports 
  ARCILATOR_ARGS += --observe-named-values 
  #ARCILATOR_ARGS += --observe-registers 
  ARCILATOR_ARGS += --observe-memories
endif

BIN := $(BUILD_DIR)/$(MODEL)-main

all: $(BIN)

$(BUILD_DIR):
	mkdir -p $@/

#===------------------------------------------------------------------------===
# SystemVerilog to core dialect IR
#===------------------------------------------------------------------------===

$(BUILD_DIR)/$(MODEL).mlir: $(MODEL).v | $(BUILD_DIR)
	circt-verilog --ir-hw --top $(TOP) $< -o $@

#===------------------------------------------------------------------------===
# Arcilator
#===------------------------------------------------------------------------===

$(BUILD_DIR)/$(MODEL)-arc.o $(BUILD_DIR)/$(MODEL).json &: $(BUILD_DIR)/$(MODEL).mlir
	arcilator $< --state-file=$(BUILD_DIR)/$(MODEL).json $(ARCILATOR_ARGS) -o $(BUILD_DIR)/$(MODEL)-arc.ll
	llc -O3 --filetype=obj $(BUILD_DIR)/$(MODEL)-arc.ll -o $(BUILD_DIR)/$(MODEL)-arc.o

$(BUILD_DIR)/$(MODEL)-arc.h: $(BUILD_DIR)/$(MODEL).json
	$(PYTHON) $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@

#===------------------------------------------------------------------------===
# Testbench
#===------------------------------------------------------------------------===

$(BIN): $(MODEL)-main.cpp $(BUILD_DIR)/$(MODEL)-arc.h $(BUILD_DIR)/$(MODEL)-arc.o
	$(CXX) $(CXXFLAGS) $< $(BUILD_DIR)/$(MODEL)-arc.o -o $@ $(LDFLAGS)

#===------------------------------------------------------------------------===
# Convenience
#===------------------------------------------------------------------------===

ir-hw: $(BUILD_DIR)/$(MODEL).mlir

arc: $(BUILD_DIR)/$(MODEL)-arc.o $(BUILD_DIR)/$(MODEL)-arc.h

run: $(BIN)
	$(BIN) $(RUN_ARGS)

# Tracing needs --trace-taps at build time, so re-enter with TRACE=1.
trace:
	$(MAKE) TRACE=1 RUN_ARGS='--trace $(MODEL).vcd' run

# Show which symbols the model exports and needs.
symbols: $(BUILD_DIR)/$(MODEL)-arc.o
	@echo "== defined =="; nm -g $< | grep -v ' U '
	@echo "== undefined =="; nm -gu $<

clean:
	rm -rf $(BUILD_DIR) $(MODEL).vcd

.PHONY: all ir-hw arc run trace symbols clean
