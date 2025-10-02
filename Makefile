IOS_SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)

bin/hello: main.c
	mkdir -p $(@D)
	clang -target arm64-apple-ios -isysroot "$(IOS_SDK)" $^ -o $@

.PHONY: clean
clean:
	rm -rf bin