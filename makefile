# Directory layout.
PROJDIR := $(realpath $(CURDIR)/)
SOURCEDIR := $(PROJDIR)/src
BUILDDIR := $(PROJDIR)/build

# Target executable
TARGET = nbm
CFLAGS = -g -Wall -Werror -O3 -std=c11
LDLIBS = -lm

# -------------------------------------------------------------------------------------------------
# enable some time functions for POSIX
CFLAGS += -D_DEFAULT_SOURCE -D_XOPEN_SOURCE

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

# Create the list of directories
DIRS = download output parser 
SOURCEDIRS = $(foreach dir, $(DIRS), $(addprefix $(SOURCEDIR)/, $(dir)))
SOURCEDIRS += $(SOURCEDIR)
TARGETDIRS = $(foreach dir, $(DIRS), $(addprefix $(BUILDDIR)/, $(dir)))
TARGETDIRS += $(BUILDDIR)

# Generate the compiler includes parameters for project generated header files.
INCLUDES = $(foreach dir, $(DIRS), $(addprefix -I, $(dir)))

# Add this list to the VPATH, the place make will look for the source files
VPATH = $(SOURCEDIRS)

# Create a list of *.c files in DIRS
SOURCES = $(foreach dir, $(SOURCEDIRS), $(wildcard $(dir)/*.c))

# Define object files for all sources, and dependencies for all objects
OBJS := $(subst $(SOURCEDIR), $(BUILDDIR), $(SOURCES:.c=.o))
DEPS = $(OBJS:.o=.d)

# Hide or not the calls depending on VERBOSE
ifeq ($(VERBOSE),TRUE)
	HIDE = 
else
	HIDE = @
endif

# Define function that will generate each rule
define generateRules
$(1)/%.o: %.c makefile
	@echo Building $$@
	$(HIDE)$(CC) -c $$(INCLUDES) $$(CFLAGS) -o $$@ $$< -MMD
endef

.PHONY: all clean directories

all: makefile directories $(TARGET)

$(TARGET): $(OBJS) makefile
	@echo Linking $@
	$(HIDE)$(CC) $(OBJS) $(LDLIBS) -o $(TARGET)

-include $(DEPS)

# Generate rules
$(foreach targetdir, $(TARGETDIRS), $(eval $(call generateRules, $(targetdir))))

directories:
	@echo Creating directory $<
	$(HIDE)mkdir -p $(TARGETDIRS) 2>/dev/null

clean:
	$(HIDE)rm -rf $(TARGETDIRS) 2>/dev/null
	$(HIDE)rm -rf $(TARGET) 2>/dev/null
	@echo Cleaning done!

