# Project Name
TARGET = DubbyPlayground

# Sources
CPP_SOURCES = \ DubbyPlayground.cpp \ Dubby.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Print floats in PrintLine()
LDFLAGS += -u _printf_float