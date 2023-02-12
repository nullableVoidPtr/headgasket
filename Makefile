PATH  := $(PATH):$(PWD)/depot_tools:/my/other/path
SHELL := env PATH=$(PATH) /bin/bash

CFLAGS = -std=c++20 -g -fno-rtti
BASE_INCLUDES = -I. -Iv8 -Iv8/include
BASE_DEFINES = -DV8_INTL_SUPPORT -DV8_ENABLE_WEBASSEMBLY
SANDBOX_DEFINES = -DV8_ENABLE_SANDBOX -DV8_COMPRESS_POINTERS -DV8_EXTERNAL_CODE_SPACE
LD_FLAGS = -lv8_monolith -lv8_libbase -lv8_libplatform -ldl -pthread

BASE_GN_ARGS = dcheck_always_on=false is_component_build=false is_debug=false use_custom_libcxx=false v8_monolithic=true v8_use_external_startup_data=false
UNSANDBOXED_GN_ARGS = v8_enable_sandbox=false v8_enable_pointer_compression=false
SANDBOXED_GN_ARGS = v8_enable_sandbox=true v8_enable_pointer_compression=true

OUT_DIR = out
UNSANDBOXED_V8_OUT_DIR = $(OUT_DIR)/v8
SANDBOXED_V8_OUT_DIR = $(OUT_DIR)/v8_sandboxed

$(UNSANDBOXED_V8_OUT_DIR)/obj $(UNSANDBOXED_V8_OUT_DIR)/gen: v8
	gn gen $(UNSANDBOXED_V8_OUT_DIR) --root=v8 --args="$(BASE_GN_ARGS) $(UNSANDBOXED_GN_ARGS)"
	ninja -C $(UNSANDBOXED_V8_OUT_DIR) v8_monolith

$(SANDBOXED_V8_OUT_DIR)/obj $(SANDBOXED_V8_OUT_DIR)/gen: v8
	gn gen $(SANDBOXED_V8_OUT_DIR) --root=v8 --args="$(BASE_GN_ARGS) $(SANDBOXED_GN_ARGS)"
	ninja -C $(SANDBOXED_V8_OUT_DIR) v8_monolith

headgasket: $(UNSANDBOXED_V8_OUT_DIR)/gen $(UNSANDBOXED_V8_OUT_DIR)/obj/libv8_monolith.a $(UNSANDBOXED_V8_OUT_DIR)/obj/libv8_libbase.a $(UNSANDBOXED_V8_OUT_DIR)/obj/libv8_libplatform.a
	$(CXX) -o $@ $(BASE_DEFINES) $(CFLAGS) $(BASE_INCLUDES) -I$(UNSANDBOXED_V8_OUT_DIR)/gen main.cc -L$(UNSANDBOXED_V8_OUT_DIR)/obj $(LD_FLAGS)

headgasket_sandboxed: $(SANDBOXED_V8_OUT_DIR)/gen $(SANDBOXED_V8_OUT_DIR)/obj/libv8_monolith.a $(SANDBOXED_V8_OUT_DIR)/obj/libv8_libbase.a $(SANDBOXED_V8_OUT_DIR)/obj/libv8_libplatform.a
	$(CXX) -o $@ $(BASE_DEFINES) $(SANDBOX_DEFINES) $(CFLAGS) $(BASE_INCLUDES) -I$(SANDBOXED_V8_OUT_DIR)/gen main.cc -L$(SANDBOXED_V8_OUT_DIR)/obj $(LD_FLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
