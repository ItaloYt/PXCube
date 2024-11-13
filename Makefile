cflags := -O0 -g -fno-omit-frame-pointer -Wall -Werror -Iinclude/ -Iinclude/phoenix/

src := \
  src/pxcube.c

obj := $(src:.c=.o)
deps := $(src:.c=.d)

.PHONY: compile
compile: phoenix pxcube

.PHONY: clear
clear:
	@rm -rf $(obj) pxcube
	@cd phoenix && $(MAKE) -s clear

.PHONY: phoenix
phoenix:
	@cd phoenix && $(MAKE) -s

%.o: %.c
	@echo "Compiling '$<'"
	@clang $(cflags) -MMD -MF $(@:.o=.d) -c -o $@ $<

pxcube:
	@echo "Linking $@"
	@clang -lphoenix -lglfw3 -lm -lvulkan -Lphoenix/ -Wl,-rpath=$(shell pwd)/phoenix/ -o $@ $(obj)

# DEPENDENCIES

pxcube: phoenix/libphoenix.so phoenix/libphoenix.a $(obj)

-include $(deps)
