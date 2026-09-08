PLUGIN = butterfly~
SRC = butterfly~.c
HEADER = butterfly_real_solver.h

# MAXSDK_PATH is the root of the Cycling '74 max-sdk repository.
C74SUPPORT = $(MAXSDK_PATH)/source/max-sdk-base/c74support
DEPLOYMENT_TARGET ?= 15.0
INCLUDES = -I"$(C74SUPPORT)/max-includes" -I"$(C74SUPPORT)/msp-includes" -I"$(C74SUPPORT)"
CFLAGS = -std=c11 -O2 -fPIC -Wall -Wextra \
         -fno-fast-math -fno-associative-math -fno-unsafe-math-optimizations \
         -fno-finite-math-only -ffp-contract=off \
         -Wno-deprecated-declarations -Wno-typedef-redefinition \
         -mmacosx-version-min=$(DEPLOYMENT_TARGET) -DMAC_VERSION -DMAC_EXT \
         -DOBJ_NAME=\"$(PLUGIN)\" $(INCLUDES)
LDFLAGS = -bundle -undefined dynamic_lookup
BUILD = build
ARM_BIN = $(BUILD)/$(PLUGIN)_arm64
X86_BIN = $(BUILD)/$(PLUGIN)_x86_64
BUNDLE = ../../externals/$(PLUGIN).mxo
EXECUTABLE = $(BUNDLE)/Contents/MacOS/$(PLUGIN)

.PHONY: all check-sdk sign
all: sign

check-sdk:
	@test -f "$(C74SUPPORT)/max-includes/ext.h" || { echo "Set MAXSDK_PATH to your max-sdk checkout." >&2; exit 1; }

$(BUILD):
	mkdir -p "$(BUILD)"

$(ARM_BIN): $(SRC) $(HEADER) Makefile | $(BUILD) check-sdk
	xcrun clang $(CFLAGS) -arch arm64 -o "$@" "$(SRC)" $(LDFLAGS)

$(X86_BIN): $(SRC) $(HEADER) Makefile | $(BUILD) check-sdk
	xcrun clang $(CFLAGS) -arch x86_64 -o "$@" "$(SRC)" $(LDFLAGS)

$(EXECUTABLE): $(ARM_BIN) $(X86_BIN) Info.plist
	mkdir -p "$(BUNDLE)/Contents/MacOS"
	xcrun lipo -create "$(ARM_BIN)" "$(X86_BIN)" -output "$@"
	cp Info.plist "$(BUNDLE)/Contents/Info.plist"

# Finder metadata may be added to a newly created bundle by macOS. It is not
# executable content; remove only the two resource attributes codesign rejects.
sign: $(EXECUTABLE)
	@xattr -dr com.apple.FinderInfo "$(BUNDLE)" 2>/dev/null || true
	@xattr -dr com.apple.ResourceFork "$(BUNDLE)" 2>/dev/null || true
	codesign --force --sign - "$(BUNDLE)"
	codesign --verify --verbose=2 "$(BUNDLE)"
