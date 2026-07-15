.PHONY: clean all splash no-splash

# Force export path environment variables to nested sub-makes
export DEVKITPRO
export DEVKITPPC

PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TARGET       := $(notdir $(PROJECT_ROOT))

ifeq ($(IN_BUILD_DIR),1)

# --- BUILD DIRECTORY STAGE ---
include $(PROJECT_ROOT)/common.mk

else

# --- ROOT DIRECTORY STAGE ---
all: splash .WAIT no-splash

splash:
	@echo "Building with splash"
	@$(MAKE) --no-print-directory -f $(PROJECT_ROOT)/common.mk SPLASH=1 TARGET=$(TARGET)-splash

no-splash:
	@echo ""
	@echo "Building without splash"
	@$(MAKE) --no-print-directory -f $(PROJECT_ROOT)/common.mk SPLASH=0 TARGET=$(TARGET)-nosplash

clean:
	@echo "Cleaning up..."
	@rm -rf build-splash build-nosplash *.elf *.dol

endif
