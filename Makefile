.PHONY: all
all: build-app build-user

.PHONY: build-%
build-%:
	$(MAKE) -C $*

.PHONY: clean
clean: clean-user clean-app

.PHONY: clean-%
clean-%:
	$(MAKE) -C $* clean

.PHONY: ipa
ipa: all
	$(MAKE) -C app ipa

.PHONY: zsign
zsign:
	cd zsign/build/macos && make clean && make