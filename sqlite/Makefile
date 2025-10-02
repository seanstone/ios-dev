include ../id.mk
export CC = $(CURDIR)/../ios-cc
export CXX = $(CURDIR)/../ios-cc
export IOS_SHIMS_DIR = $(CURDIR)/../shims

SQLITE_VERSION = 3500400

SRC := sqlite-amalgamation-$(SQLITE_VERSION)/shell.c \
	   sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.c

OBJ := $(patsubst %,build/%.o,$(SRC))

CFLAGS += -Isqlite-amalgamation-$(SQLITE_VERSION)

bin/sqlite3: $(OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

build/%.c.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC): sqlite-amalgamation-$(SQLITE_VERSION)

sqlite-amalgamation-$(SQLITE_VERSION): sqlite-amalgamation-$(SQLITE_VERSION).zip
	cd $(@D) && unzip $<

sqlite-amalgamation-$(SQLITE_VERSION).zip:
	mkdir -p $(@D)
	cd $(@D) && wget https://www.sqlite.org/2025/$@

.PHONY: clean
clean:
	rm -rf bin build sqlite-amalgamation-*