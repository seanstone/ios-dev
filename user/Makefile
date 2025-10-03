.PHONY: all
all: build-hello build-sqlite build-gcat build-zsign build-bash

.PHONY: build-%
build-%:
	$(MAKE) -C $*

.PHONY: clean
clean: clean-hello clean-sqlite

clean-%:
	rm -rf bin lib
	$(MAKE) -C $* clean