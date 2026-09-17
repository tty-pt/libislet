all := libislet
LDLIBS-libislet := -lqsys -lqmap
CFLAGS := -g
CFLAGS += -I/home/quirinpa/site/external/libqmap/include

-include ../mk/include.mk

test:
	$(MAKE) -C tests test

bench:
	$(MAKE) -C tests bench
