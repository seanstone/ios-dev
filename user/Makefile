.PHONY: all
all: build-hello build-sqlite build-gcat

.PHONY: build-%
build-%:
	$(MAKE) -C $*

.PHONY: clean
clean: clean-hello clean-sqlite

clean-%:
	rm -rf bin
	$(MAKE) -C $* clean