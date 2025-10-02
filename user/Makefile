.PHONY: all
all: build-hello build-sqlite

.PHONY: build-%
build-%:
	$(MAKE) -C $*

.PHONY: clean
clean: clean-hello clean-sqlite

clean-%:
	$(MAKE) -C $* clean