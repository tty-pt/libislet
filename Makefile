all := libislet
LDLIBS-libislet := -lqsys -lcorm
CFLAGS := -g
add-prefix := ../libcorm

-include ../mk/include.mk

test:
	$(MAKE) -C tests test

bench:
	$(MAKE) -C tests bench
