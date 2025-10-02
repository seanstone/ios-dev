.PHONY: default
default: bin/libios-dev.a bin/hello

include id.mk

CC = $(CURDIR)/ios-cc

bin/libios-dev.a: build/app/app-main.c.o
	mkdir -p $(@D)
	ar rcs $@ $^

bin/hello: build/hello/main.c.o
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

build/%.c.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: ipa
ipa: app/export/ios-dev.ipa

app/ios-dev.xcarchive: bin/libios-dev.a bin/hello
	cd app && xcodebuild archive -scheme ios-dev -sdk iphoneos -allowProvisioningUpdates -archivePath ios-dev.xcarchive

app/export/ios-dev.ipa: app/ios-dev.xcarchive
	cd app && xcodebuild -exportArchive -archivePath ios-dev.xcarchive -exportPath export -exportOptionsPlist ExportOptions.plist

.PHONY: clean
clean:
	rm -rf bin build app/*.xcarchive app/export

include sqlite.mk