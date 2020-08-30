# Directory layout.
PROJDIR := $(realpath $(CURDIR)/)
SOURCEDIR := $(PROJDIR)/src
BUILDDIR := $(PROJDIR)/build

# Target executable
TARGET = nbm
CFLAGS = -g -Wall -Werror -O3
LDLIBS = -lm

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

RM = rm -rf
RMDIR = rm -rf
MKDIR = mkdir -p
ERRIGNORE = 2>/dev/null
SEP=/

# Remove space after separator
PSEP = $(strip $(SEP))

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
	$(HIDE)$(CC) -c $$(INCLUDES) $$(CFLAGS) -o $$(subst /,$$(PSEP),$$@) $$(subst /,$$(PSEP),$$<) -MMD
endef

.PHONY: all clean directories

all: makefile directories $(TARGET)

$(TARGET): $(OBJS) makefile
	$(HIDE)echo Linking $@
	$(HIDE)$(CC) $(OBJS) $(LDLIBS) -o $(TARGET)

-include $(DEPS)

# Generate rules
$(foreach targetdir, $(TARGETDIRS), $(eval $(call generateRules, $(targetdir))))

directories:
	$(HIDE)echo Creating directory $<
	$(HIDE)$(MKDIR) $(subst /,$(PSEP),$(TARGETDIRS)) $(ERRIGNORE)

clean:
	$(HIDE)$(RMDIR) $(subst /,$(PSEP),$(TARGETDIRS)) $(ERRIGNORE)
	$(HIDE)$(RM) $(TARGET) $(ERRIGNORE)
	@echo Cleaning done !

