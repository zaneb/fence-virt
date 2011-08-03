bin=libvirt-qmf_test
SOURCES=test_main.c tests.c
INCLUDE_DIRS=../include
LIBRARIES=dl

CFLAGS=$(addprefix -I,$(INCLUDE_DIRS)) -Wall
LDFLAGS=$(addprefix -l,$(LIBRARIES)) -Wall
objs=$(SOURCES:.c=.o)

plugin=../server/libvirt-qpid.so

test: all
	$$(pwd)/$(bin)

debug: all
	gdb $(bin)

all: $(bin) $(plugin);

$(bin): $(objs)
	gcc $^ -o $@ $(LDFLAGS)

$(objs): %.o:%.c
	gcc -c $< -o $@ $(CFLAGS)

$(plugin): ../server/libvirt-qpid.cpp
	$(MAKE) -C ..

clean:
	rm -f $(objs) $(bin)

.PHONY: all test clean
