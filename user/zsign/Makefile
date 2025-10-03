../bin/zsign:
	cd zsign/build/macos && make
	cp zsign/bin/zsign ../bin/
	mkdir -p $(@D)
	./zsign-test.sh