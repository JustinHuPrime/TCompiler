# Copyright 2020 Justin Hu
#
# This file is part of the T Language Compiler.
#
# The T Language Compiler is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# The T Language Compiler is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# the T Language Compiler. If not, see <https://www.gnu.org/licenses/>.
# 
# SPDX-License-Identifier: GPL-3.0-or-later

# command options
CC := gcc
RM := rm -rf
MV := mv
MKDIR := mkdir -p
DOXYGEN := doxygen
TOUCH := touch
SED := sed
ECHO := echo
PDFLATEX := pdflatex


# File options
SRCDIRPREFIX := src
OBJDIRPREFIX := bin
DEPDIRPREFIX := dependencies
MAINSUFFIX := main
TESTSUFFIX := test
DOCSDIR := docs
STANDARDDIR := standard

# Main file options
SRCDIR := $(SRCDIRPREFIX)/$(MAINSUFFIX)
SRCS := $(shell find -O3 $(SRCDIR) -type f -name '*.c')

OBJDIR := $(OBJDIRPREFIX)/$(MAINSUFFIX)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

DEPDIR := $(DEPDIRPREFIX)/$(MAINSUFFIX)
DEPS := $(patsubst $(SRCDIR)/%.c,$(DEPDIR)/%.dep,$(SRCS))

# Test file options
TSRCDIR := $(SRCDIRPREFIX)/$(TESTSUFFIX)
TSRCS := $(shell find -O3 $(TSRCDIR) -type f -name '*.c')

TOBJDIR := $(OBJDIRPREFIX)/$(TESTSUFFIX)
TOBJS := $(patsubst $(TSRCDIR)/%.c,$(TOBJDIR)/%.o,$(TSRCS))

TDEPDIR := $(DEPDIRPREFIX)/$(TESTSUFFIX)
TDEPS := $(patsubst $(TSRCDIR)/%.c,$(TDEPDIR)/%.dep,$(TSRCS))


# final executable name
EXENAME := tlc
TEXENAME := tlc-test


# compiler warnings
WARNINGS := -pedantic -pedantic-errors -Wall -Wextra -Wdouble-promotion\
-Winit-self -Wunused -Wswitch-unreachable -Wuninitialized\
-Wstringop-truncation -Wsuggest-attribute=format -Wsuggest-attribute=malloc\
-Wmissing-include-dirs -Wswitch-bool -Wduplicated-branches -Wduplicated-cond\
-Wtrampolines -Wfloat-equal -Wundef -Wunsafe-loop-optimizations\
-Wunused-macros -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings\
-Wconversion  -Wdate-time -Wjump-misses-init -Wlogical-op -Waggregate-return\
-Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes\
-Wmissing-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline\
-Winvalid-pch -Wdisabled-optimization -Wstack-protector\
-Wunsuffixed-float-constants

# compiler options
OPTIONS := -std=c18 -m64 -D_POSIX_C_SOURCE=202002L -I$(SRCDIR) $(WARNINGS)\
-fPIE -pie
DEBUGOPTIONS := -Og -ggdb -Wno-unused-parameter
RELEASEOPTIONS := -O3 -DNDEBUG
COVERAGEOPTIONS := --coverage
TOPTIONS := -I$(TSRCDIR)
LIBS :=


.PHONY: debug release clean diagnose docs
.SECONDEXPANSION:
.SUFFIXES:

# entry points
debug: OPTIONS := $(OPTIONS) $(DEBUGOPTIONS)
debug: $(EXENAME) $(TEXENAME) docs
	@$(ECHO) "Running tests"
	@./$(TEXENAME)
	@$(ECHO) "Done building debug!"

release: OPTIONS := $(OPTIONS) $(RELEASEOPTIONS)
release: $(EXENAME) $(TEXENAME) docs
	@$(ECHO) "Running tests"
	@./$(TEXENAME)
	@$(ECHO) "Done building release!"

coverage: OPTIONS := $(OPTIONS) $(COVERAGEOPTIONS)
coverage: $(TEXENAME)
	@$(ECHO) "Running tests"
	@./$(TEXENAME) 2> /dev/null
	@$(ECHO) "Test coverage generated!"

docs: $(DOCSDIR)/.timestamp $(STANDARDDIR)/Standard.pdf

clean:
	@$(ECHO) "Removing all generated files and folders."
	@$(RM) $(OBJDIRPREFIX) $(DEPDIRPREFIX) $(DOCSDIR) $(shell find -O3 $(STANDARDDIR) ! '(' -name '*.tex' ')' -name 'Standard.*') $(EXENAME) $(TEXENAME)


# documentation details
$(DOCSDIR)/.timestamp: $(shell find -O3 $(SRCDIR) -type f -name '*.[ch]')
	@$(ECHO) "Generating documentation"
	@$(DOXYGEN)
	@$(TOUCH) $@

$(STANDARDDIR)/Standard.pdf: $(STANDARDDIR)/Standard.tex
	@$(ECHO) "Compiling standard"
	@$(PDFLATEX) -output-directory $(STANDARDDIR) $< > /dev/null
	@$(PDFLATEX) -output-directory $(STANDARDDIR) $< > /dev/null


# executable details
$(EXENAME): $(OBJS)
	@$(ECHO) "Linking $@"
	@$(CC) -o $(EXENAME) $(OPTIONS) $(OBJS) $(LIBS)

$(OBJS): $$(patsubst $(OBJDIR)/%.o,$(SRCDIR)/%.c,$$@) $$(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.dep,$$@) | $$(dir $$@)
	@$(ECHO) "Compiling $@"
	@$(CC) $(OPTIONS) -c $< -o $@

$(DEPS): $$(patsubst $(DEPDIR)/%.dep,$(SRCDIR)/%.c,$$@) | $$(dir $$@)
	@set -e; $(RM) $@; \
	 $(CC) $(OPTIONS) -MM -MT $(patsubst $(DEPDIR)/%.dep,$(OBJDIR)/%.o,$@) $< > $@.$$$$; \
	 $(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 $(RM) $@.$$$$


# test details
$(TEXENAME): $(TOBJS) $(OBJS)
	@$(ECHO) "Linking $@"
	@$(CC) -o $(TEXENAME) $(OPTIONS) $(TOPTIONS) $(filter-out %main.o,$(OBJS)) $(TOBJS) $(LIBS)

$(TOBJS): $$(patsubst $(TOBJDIR)/%.o,$(TSRCDIR)/%.c,$$@) $$(patsubst $(TOBJDIR)/%.o,$(TDEPDIR)/%.dep,$$@) | $$(dir $$@)
	@$(ECHO) "Compiling $@"
	@$(CC) $(OPTIONS) $(TOPTIONS) -c $< -o $@

$(TDEPS): $$(patsubst $(TDEPDIR)/%.dep,$(TSRCDIR)/%.c,$$@) | $$(dir $$@)
	@set -e; $(RM) $@; \
	 $(CC) $(OPTIONS) $(TOPTIONS) -MM -MT $(patsubst $(TDEPDIR)/%.dep,$(TOBJDIR)/%.o,$@) $< > $@.$$$$; \
	 $(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 $(RM) $@.$$$$


%/:
	@$(MKDIR) $@


-include $(DEPS) $(TDEPS)