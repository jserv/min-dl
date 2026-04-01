CROSS_COMPILE_SUFFIX ?= -linux-gnueabi
CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -std=gnu99 -Wall -Werror -g -D_GNU_SOURCE -DPROG_HEADER=prog_header

OUT = out
ARCH = arm aarch64 x86_64

all: $(OUT)/testlib.so $(OUT)/loader $(OUT)/symlib.so $(OUT)/symtest $(OUT)/initlib.so $(OUT)/inittest

$(OUT):
	@mkdir -p $@

$(OUT)/%.o: %.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ -MMD -MF $@.d -c $<

$(OUT)/testlib.o: CFLAGS += -fvisibility=hidden -fPIC

$(OUT)/testlib.so: $(OUT)/testlib.o
	$(CC) -shared -Wl,--entry=prog_header -Wl,-z,defs -nostdlib $< -o $@

$(OUT)/loader: $(OUT)/loader.o $(OUT)/testloader.o
	$(CC) -o $@ $^

$(OUT)/symlib.o: CFLAGS += -fPIC

$(OUT)/symlib.so: $(OUT)/symlib.o
	$(CC) -shared -nostdlib $< -o $@

$(OUT)/symtest: $(OUT)/loader.o $(OUT)/symtest.o
	$(CC) -o $@ $^

$(OUT)/initlib.o: CFLAGS += -fPIC

$(OUT)/initlib.so: $(OUT)/initlib.o
	$(CC) -shared -nostdlib $< -o $@

$(OUT)/inittest: $(OUT)/loader.o $(OUT)/inittest.o
	$(CC) -o $@ $^

# Cross-compilation: 'make arm' or 'make aarch64'
$(ARCH): %: check_cc_%
	@$(MAKE) CROSS_COMPILE=$@$(CROSS_COMPILE_SUFFIX)- all

$(addprefix check_cc_,$(ARCH))::
	@which $(patsubst check_cc_%,%,$@)$(CROSS_COMPILE_SUFFIX)-gcc >/dev/null

# ld >= 2.28 required: older versions corrupt global arrays in shared objects.
check_cc_aarch64::
	@ld_ver=$$(aarch64$(CROSS_COMPILE_SUFFIX)-ld -v | grep -oE '[0-9]+\.[0-9]+' | head -1); \
	major=$$(echo "$$ld_ver" | cut -d. -f1); minor=$$(echo "$$ld_ver" | cut -d. -f2); \
	if [ "$$major$$minor" -lt 228 ] 2>/dev/null; then \
		echo "Error: aarch64 ld must be >= 2.28 (found $$ld_ver)"; exit 1; \
	fi

$(addprefix check_,$(ARCH)): check_%: %
	@cd $(OUT) && qemu-$* -L /usr/$*$(CROSS_COMPILE_SUFFIX)/ ./loader
	@cd $(OUT) && qemu-$* -L /usr/$*$(CROSS_COMPILE_SUFFIX)/ ./symtest
	@cd $(OUT) && qemu-$* -L /usr/$*$(CROSS_COMPILE_SUFFIX)/ ./inittest

check: all
	@cd $(OUT) && ./loader
	@cd $(OUT) && ./symtest
	@cd $(OUT) && ./inittest

clean:
	rm -rf $(OUT)

SRCS = loader.c testloader.c testlib.c symtest.c symlib.c inittest.c initlib.c
-include $(SRCS:%.c=$(OUT)/%.o.d)

.PHONY: all clean check $(ARCH) $(addprefix check_,$(ARCH)) $(addprefix check_cc_,$(ARCH))
