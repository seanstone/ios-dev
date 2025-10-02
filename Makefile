.PHONY: default
default: bin/libios-env.a bin/hello

IOS_SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

bin/libios-env.a: app/app-main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot "$(IOS_SDK)" -c $< -o bin/app-main.o
	ar rcs $@ bin/app-main.o

bin/hello: hello/main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot "$(IOS_SDK)" $^ -o $@

.PHONY: clean
clean:
	rm -rf bin