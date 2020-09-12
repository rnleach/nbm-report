# Directory layout.
PROJDIR := $(realpath $(CURDIR)/)
SOURCEDIR := $(PROJDIR)/src
OBJDIR := $(PROJDIR)/obj
BUILDDIR := $(PROJDIR)/build

# Target executable
TARGET = $(BUILDDIR)/nbm
CFLAGS = -g -flto -Wall -Werror -O3 -std=c11 -I$(SOURCEDIR)
LDLIBS = -flto -lm

# -------------------------------------------------------------------------------------------------
# enable some time functions for POSIX
CFLAGS += -D_DEFAULT_SOURCE -D_XOPEN_SOURCE

# glib
CFLAGS += `pkg-config --cflags glib-2.0`
LDLIBS += `pkg-config --libs glib-2.0`

# cURL library
CFLAGS += `curl-config --cflags`
LDLIBS += `curl-config --libs`

# libcsv3 library
CFLAGS +=
LDLIBS += -lcsv
# -------------------------------------------------------------------------------------------------

# Compiler and compiler options
CC = clang

# Show commands make uses
VERBOSE = TRUE

# Add this list to the VPATH, the place make will look for the source files
VPATH = $(SOURCEDIR)

# Create a list of *.c files in DIRS
SOURCES = $(wildcard $(SOURCEDIR)/*.c)

# Define object files for all sources, and dependencies for all objects
OBJS := $(subst $(SOURCEDIR), $(OBJDIR), $(SOURCES:.c=.o))
DEPS = $(OBJS:.o=.d)

# Hide or not the calls depending on VERBOSE
ifeq ($(VERBOSE),TRUE)
	HIDE = 
else
	HIDE = @
endif

.PHONY: all clean directories 

all: makefile directories $(TARGET)

$(TARGET): directories makefile $(OBJS) 
	@echo Linking $@
	$(HIDE)$(CC) $(OBJS) $(LDLIBS) -o $(TARGET)

-include $(DEPS)

# Generate rules
$(OBJDIR)/%.o: $(SOURCEDIR)/%.c makefile
	@echo Building $@
	$(HIDE)$(CC) -c $(CFLAGS) -o $@ $< -MMD

directories:
	@echo Creating directory $<
	$(HIDE)mkdir -p $(OBJDIR) 2>/dev/null
	$(HIDE)mkdir -p $(BUILDDIR) 2>/dev/null

clean:
	$(HIDE)rm -rf $(OBJDIR) $(BUILDDIR) 2>/dev/null
	@echo Cleaning done!

detected_OS = $(shell uname)
ifeq ($(detected_OS), Linux)
	target_dir = ~/usr/bin/
else
	target_dir = ~/bin/
endif

install: $(TARGET) makefile
	strip $(TARGET) && cp $(TARGET) $(target_dir)

