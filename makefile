# Copyright 2019 Justin Hu
#
# This file is part of the T Language Compiler.

# command options
CC := gcc
RM := rm -rf
MV := mv
MKDIR := mkdir -p
FORMATTER := clang-format -i


# File options
SRCDIRPREFIX := src
OBJDIRPREFIX := bin
DEPDIRPREFIX := dependencies
MAINSUFFIX := main
TESTSUFFIX := test

# Main file options
SRCDIR := $(SRCDIRPREFIX)/$(MAINSUFFIX)
SRCS := $(shell find -O3 $(SRCDIR)/ -type f -name '*.c')

OBJDIR := $(OBJDIRPREFIX)/$(MAINSUFFIX)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

DEPDIR := $(DEPDIRPREFIX)/$(MAINSUFFIX)
DEPS := $(patsubst $(SRCDIR)/%.c,$(DEPDIR)/%.dep,$(SRCS))

# Test file options
TSRCDIR := $(SRCDIRPREFIX)/$(TESTSUFFIX)
TSRCS := $(shell find -O3 $(TSRCDIR)/ -type f -name '*.c')

TOBJDIR := $(OBJDIRPREFIX)/$(TESTSUFFIX)
TOBJS := $(patsubst $(TSRCDIR)/%.c,$(TOBJDIR)/%.o,$(TSRCS))

TDEPDIR := $(DEPDIRPREFIX)/$(TESTSUFFIX)
TDEPS := $(patsubst $(TSRCDIR)/%.c,$(TDEPDIR)/%.dep,$(TSRCS))


# final executable name
EXENAME := tlc
TEXENAME := tlc-test


# compiler warnings
WARNINGS := -pedantic -pedantic-errors -Wall -Wextra -Wdouble-promotion\
-Winit-self -Wmissing-include-dirs -Wswitch-enum -Wtrampolines -Wfloat-equal\
-Wundef -Wshadow -Wunsafe-loop-optimizations -Wbad-function-cast -Wcast-qual\
-Wcast-align -Wwrite-strings -Wconversion -Wjump-misses-init -Wlogical-op\
-Waggregate-return -Wstrict-prototypes -Wold-style-definition\
-Wmissing-prototypes -Wmissing-declarations -Wmissing-format-attribute\
-Wpacked -Wnested-externs -Winline -Winvalid-pch -Wdisabled-optimization\
-Wstack-protector

# compiler options
OPTIONS := -std=c18 -m64 -D_POSIX_C_SOURCE=201803L -I$(SRCDIR) $(WARNINGS)
DEBUGOPTIONS := -Og -ggdb -Wno-unused
RELEASEOPTIONS := -O3 -D NDEBUG -Wunused
TOPTIONS := -I$(TSRCDIR)
LIBS :=


.PHONY: debug release clean diagnose
.SECONDEXPANSION:
.SUFFIXES:

debug: OPTIONS := $(OPTIONS) $(DEBUGOPTIONS)
debug: $(EXENAME) $(TEXENAME)
	@echo "Running tests"
	@./$(TEXENAME)
	@echo "Done building debug!"

release: OPTIONS := $(OPTIONS) $(RELEASEOPTIONS)
release: $(EXENAME) $(TEXENAME)
	@echo "Running tests"
	@./$(TEXENAME)
	@echo "Done building release!"


clean:
	@echo "Removing all generated files and folders."
	@$(RM) $(OBJDIRPREFIX) $(DEPDIRPREFIX) $(EXENAME) $(TEXENAME)


$(EXENAME): $(OBJS) $(GENERATEDOBJS)
	@echo "Linking $@"
	@$(CC) -o $(EXENAME) $(OPTIONS) $(OBJS) $(LIBS)

$(OBJS): $$(patsubst $(OBJDIR)/%.o,$(SRCDIR)/%.c,$$@) $$(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.dep,$$@) | $$(dir $$@)
	@echo "Compiling $@"
	@$(FORMATTER) $(filter-out %.dep,$^)
	@$(CC) $(OPTIONS) -c $< -o $@

$(DEPS): $$(patsubst $(DEPDIR)/%.dep,$(SRCDIR)/%.c,$$@) | $$(dir $$@)
	@set -e; $(RM) $@; \
	 $(CC) $(OPTIONS) -MM -MT $(patsubst $(DEPDIR)/%.dep,$(OBJDIR)/%.o,$@) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$


$(TEXENAME): $(TOBJS) $(OBJS) $(GENERATEDOBJS)
	@echo "Linking $@"
	@$(CC) -o $(TEXENAME) $(OPTIONS) $(TOPTIONS) $(filter-out %main.o,$(OBJS)) $(GENERATEDOBJS) $(TOBJS) $(LIBS)

$(TOBJS): $$(patsubst $(TOBJDIR)/%.o,$(TSRCDIR)/%.c,$$@) $$(patsubst $(TOBJDIR)/%.o,$(TDEPDIR)/%.dep,$$@) | $$(dir $$@)
	@echo "Compiling $@"
	@$(FORMATTER) $(filter-out %.dep,$^)
	@$(CC) $(OPTIONS) $(TOPTIONS) -c $< -o $@

$(TDEPS): $$(patsubst $(TDEPDIR)/%.dep,$(TSRCDIR)/%.c,$$@) | $$(dir $$@)
	@set -e; $(RM) $@; \
	 $(CC) $(OPTIONS) $(TOPTIONS) -MM -MT $(patsubst $(TDEPDIR)/%.dep,$(TOBJDIR)/%.o,$@) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$


%/:
	@$(MKDIR) $@


-include $(DEPS) $(TDEPS)