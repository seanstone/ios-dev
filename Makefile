IOS_SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

bin/libios-env.a:
	mkdir -p $(@D)
	clang -c -arch arm64 -isysroot "$(IOS_SDK)" -c app/main.c -o bin/main.o
	cd bin && ar rcs libios-env.a main.o

bin/hello: main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot "$(IOS_SDK)" $^ -o $@

.PHONY: clean
clean:
	rm -rf bin