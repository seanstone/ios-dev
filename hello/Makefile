include ../id.mk
export CC = $(CURDIR)/../ios-cc
export CXX = $(CURDIR)/../ios-cc
export IOS_SHIMS_DIR = $(CURDIR)/../shims

SRC := main.c
OBJ := $(patsubst %,build/%.o,$(SRC))

bin/hello: $(OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

build/%.c.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf bin build