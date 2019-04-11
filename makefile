# Copyright 2019 Justin Hu
#
# This file is part of the T Language Compiler.

# command options
CC := gcc
RM := rm -rf
MV := mv
MKDIR := mkdir -p
FORMATTER := clang-format -i
YACC := bison
LEX := flex


# File options
SRCDIRPREFIX := src
OBJDIRPREFIX := bin
DEPDIRPREFIX := dependencies

SRCDIR := $(SRCDIRPREFIX)/main
GENERATEDSOURCES := $(SRCDIR)/parser/impl/lex.yy.c $(SRCDIR)/parser/impl/lex.yy.h $(SRCDIR)/parser/impl/parser.tab.c $(SRCDIR)/parser/impl/parser.tab.h
FILESRCS := $(shell find -O3 $(SRCDIR)/ -not -path "$(SRCDIR)/parser/impl/parser.tab.c" -not -path "$(SRCDIR)/parser/impl/lex.yy.c" -type f -name '*.c')
SRCS := $(FILESRCS) $(filter-out %.h, $(GENERATEDSOURCES))

OBJDIR := $(OBJDIRPREFIX)/main
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(FILESRCS))
GENERATEDOBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(filter-out %.h, $(GENERATEDSOURCES)))

DEPDIR := $(DEPDIRPREFIX)/main
DEPS := $(patsubst $(SRCDIR)/%.c,$(DEPDIR)/%.dep,$(FILESRCS))


# Test file options
TSRCDIR := $(SRCDIRPREFIX)/test
TSRCS := $(shell find -O3 $(TSRCDIR)/ -type f -name '*.c')

TOBJDIR := $(OBJDIRPREFIX)/test
TOBJS := $(patsubst $(TSRCDIR)/%.c,$(TOBJDIR)/%.o,$(TSRCS))

TDEPDIR := $(DEPDIRPREFIX)/test
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


# lex options
LEXOPTIONS :=  --header-file=$(SRCDIR)/parser/impl/lex.yy.h --outfile=$(SRCDIR)/parser/impl/lex.yy.c


# yacc options
YACCOPTIONS := --defines=$(SRCDIR)/parser/impl/parser.tab.h --output=$(SRCDIR)/parser/impl/parser.tab.c --report=state


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
	@$(RM) $(OBJDIRPREFIX) $(DEPDIRPREFIX) $(EXENAME) $(TEXENAME) $(GENERATEDSOURCES) $(SRCDIR)/parser/impl/parser.output


$(EXENAME): $(OBJS) $(GENERATEDOBJS)
	@echo "Linking $@"
	@$(CC) -o $(EXENAME) $(OPTIONS) $(OBJS) $(GENERATEDOBJS) $(LIBS)

$(OBJS): $$(patsubst $(OBJDIR)/%.o,$(SRCDIR)/%.c,$$@) $$(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.dep,$$@) | $$(dir $$@)
	@echo "Compiling $@"
	@$(FORMATTER) $(filter-out %.dep,$^)
	@$(CC) $(OPTIONS) -c $< -o $@

$(GENERATEDOBJS): $$(patsubst $(OBJDIR)/%.o,$(SRCDIR)/%.c,$$@) | $$(dir $$@)
	@echo "Compiling $@"
	@$(CC) $(OPTIONS) -c $< -o $@

$(SRCDIR)/parser/impl/lex.yy.c: $(SRCDIR)/parser/impl/lexer.l $(SRCDIR)/parser/impl/parser.tab.h
	@echo "Creating $@"
	@$(LEX) $(LEXOPTIONS) $(SRCDIR)/parser/impl/lexer.l

$(SRCDIR)/parser/impl/parser.tab.c $(SRCDIR)/parser/impl/parser.tab.h: $(SRCDIR)/parser/impl/parser.y
	@echo "Creating $@"
	@$(YACC) $(YACCOPTIONS) src/main/parser/impl/parser.y

$(DEPS): $$(patsubst $(DEPDIR)/%.dep,$(SRCDIR)/%.c,$$@) $(GENERATEDSOURCES) | $$(dir $$@)
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