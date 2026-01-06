PLUGIN = cusp~
SRC    = cusp~.c

# Max SDK support folder:
# - If MAXSDK_PATH is set, it should point to the root of the Cycling '74 max-sdk repository.
#   (Expected includes path: $(MAXSDK_PATH)/source/max-sdk-base/c74support)
# - Otherwise we fall back to a local checkout at ../../max-sdk-base/c74support (developer convenience).
ifeq ($(strip $(MAXSDK_PATH)),)
SDKROOT     := $(abspath ../..)
C74SUPPORT  := $(SDKROOT)/max-sdk-base/c74support
else
C74SUPPORT  := $(MAXSDK_PATH)/source/max-sdk-base/c74support
endif

INCLUDES = -I"$(C74SUPPORT)/max-includes" \
           -I"$(C74SUPPORT)/msp-includes" \
           -I"$(C74SUPPORT)"

CFLAGS  = -std=c99 -fPIC -O2 \
          -Wno-deprecated-declarations -Wno-typedef-redefinition \
          -DMAC_VERSION -DMAC_EXT \
          -DOBJ_NAME=\"$(PLUGIN)\" \
          $(INCLUDES)

LDFLAGS = -bundle -undefined dynamic_lookup

BUILD    = build
ARM_BIN  = $(BUILD)/$(PLUGIN)_arm64
X86_BIN  = $(BUILD)/$(PLUGIN)_x86_64
UNIV_BIN = $(BUILD)/$(PLUGIN)_univ

all: $(PLUGIN).mxo

$(BUILD):
	mkdir -p $(BUILD)

$(ARM_BIN): $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) -arch arm64  -o $@ $< $(LDFLAGS)

$(X86_BIN): $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) -arch x86_64 -o $@ $< $(LDFLAGS)

$(UNIV_BIN): $(ARM_BIN) $(X86_BIN)
	lipo -create -output $@ $^

$(PLUGIN).mxo: $(UNIV_BIN)
	rm -rf $@
	mkdir -p $@/Contents/MacOS
	cp $< $@/Contents/MacOS/$(PLUGIN)
	mkdir -p $@/Contents
	printf '%s\n' \
		'<?xml version="1.0" encoding="UTF-8"?>' \
		'<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
		'<plist version="1.0">' \
		'<dict>' \
		'  <key>CFBundleExecutable</key><string>$(PLUGIN)</string>' \
		'  <key>CFBundleIdentifier</key><string>com.leon.catastrophes.$(PLUGIN)</string>' \
		'  <key>CFBundleShortVersionString</key><string>1.0.0</string>' \
		'  <key>CFBundleVersion</key><string>1.0.0</string>' \
		'</dict>' \
		'</plist>' > $@/Contents/Info.plist

clean:
	rm -rf $(BUILD) $(PLUGIN).mxo
