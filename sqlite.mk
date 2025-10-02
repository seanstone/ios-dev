SQLITE_VERSION = 3500400

third-party/sqlite-amalgamation-$(SQLITE_VERSION): third-party/sqlite-amalgamation-$(SQLITE_VERSION).zip
	cd $(@D) && unzip sqlite-amalgamation-$(SQLITE_VERSION).zip

third-party/sqlite-amalgamation-$(SQLITE_VERSION).zip:
	mkdir -p $(@D)
	cd $(@D) && wget https://www.sqlite.org/2025/sqlite-amalgamation-$(SQLITE_VERSION).zip

SQLITE_OBJ = build/third-party/sqlite-amalgamation-$(SQLITE_VERSION)/shell.c.o \
	build/third-party/sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.c.o \

CFLAGS += -Ithird-party/sqlite-amalgamation-$(SQLITE_VERSION)

.PHONY: sqlite
sqlite: bin/sqlite3

bin/sqlite3: $(SQLITE_OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@