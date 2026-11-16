all := libislet
LDLIBS-libislet := -lstele -lcorm
CFLAGS := -g

WASM_SRCS := npm/wasm/islet_wasm.c

-include ../mk/include.mk

test: all
	$(MAKE) -C tests test

bench:
	$(MAKE) -C tests bench

# Optional Node.js & WebAssembly bindings
NPM ?= npm

npm-build:
	@if command -v $(NPM) >/dev/null 2>&1; then \
		(cd npm && $(NPM) run build); \
	else \
		echo "Skipping npm build — $(NPM) not found"; \
	fi

npm-test: npm-build
	@if command -v $(NPM) >/dev/null 2>&1; then \
		(cd npm && $(NPM) test); \
	fi

