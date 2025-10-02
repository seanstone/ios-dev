.PHONY: default
default: bin/libios-env.a bin/hello

IOS_SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

bin/libios-env.a:
	mkdir -p $(@D)
	clang -c -arch arm64 -isysroot "$(IOS_SDK)" -c app/ios-env.c -o bin/ios-env.o
	ar rcs $@ bin/ios-env.o

bin/hello: hello/main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot "$(IOS_SDK)" $^ -o $@

.PHONY: clean
clean:
	rm -rf bin