include ../../toolchain/id.mk
export CC = $(CURDIR)/../../toolchain/ios-cc
export CXX = $(CURDIR)/../../toolchain/ios-cc
export IOS_SYSCALLS_DIR = $(CURDIR)/../../toolchain/syscalls

SRC := main.c
OBJ := $(patsubst %,build/%.o,$(SRC))

../bin/hello: $(OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

build/%.c.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf bin build