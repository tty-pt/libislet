all := libislet
LDLIBS-libislet := -lqsys -lcorm
CFLAGS := -g
CFLAGS += -I/home/quirinpa/site/external/libcorm/include

-include ../mk/include.mk

test:
	$(MAKE) -C tests test

bench:
	$(MAKE) -C tests bench
