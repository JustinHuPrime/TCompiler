# Copyright 2019 Justin Hu
#
# This file is part of the T Compiler.

# command options
CC := gcc
RM := rm -rf
MV := mv
MKDIR := mkdir -p
FORMATTER := clang-format
YACC := bison
LEX := flex


# File options
SRCDIRPREFIX := src
OBJDIRPREFIX := bin
DEPDIRPREFIX := dependencies

SRCDIR := $(SRCDIRPREFIX)/main
GENERATEDSOURCES := $(SRCDIR)/parser/lex.yy.c $(SRCDIR)/parser/parser.tab.c $(SRCDIR)/parser/parser.tab.h
FILESRCS := $(shell find -O3 $(SRCDIR)/ -not -path "$(SRCDIR)/parser/parser.tab.[ch]" -not -path "$(SRCDIR)/parser/lex.yy.c" -type f -name '*.c')
SRCS := $(FILESRCS) $(filter-out %.h, $(GENERATEDSOURCES))

OBJDIR := $(OBJDIRPREFIX)/main
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

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
EXENAME := tcc
TEXENAME := tcc-test


# compiler warnings
WARNINGS := -Wall -Wextra -Wpedantic -Wpedantic-errors -Wmissing-include-dirs\
-Wswitch-default -Wuninitialized -Wstrict-overflow=5 -Wsuggest-override\
-Wfloat-equal -Wshadow -Wundef -Wunused-macros -Wcast-qual -Wcast-align=strict\
-Wconversion -Wzero-as-null-pointer-constant -Wformat=2 -Wuseless-cast\
-Wextra-semi -Wsign-conversion -Wlogical-op -Wmissing-declarations\
-Wredundant-decls -Winline -Winvalid-pch -Wdisabled-optimization\
-Wstrict-null-sentinel -Wsign-promo -Wbad-function-cast -Wjump-misses-init\
-Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes\
-Wnormalized=nfc -Wpadded -Wnested-externs

# compiler options
OPTIONS := -std=c18 -m64 -D_POSIX_C_SOURCE=201803L -I$(SRCDIR)
DEBUGOPTIONS := -Og -ggdb -Wno-unused
RELEASEOPTIONS := -O3 -D NDEBUG -Wunused
TOPTIONS := -I$(TSRCDIR)
LIBS :=


# lex options
LEXOPTIONS :=


# yacc options
YACCOPTIONS := -d -v --report=state


DIAGNOSE := UNSET

.PHONY: debug release clean diagnose
.SECONDEXPANSION:
.SUFFIXES:

debug: OPTIONS := $(OPTIONS) $(DEBUGOPTIONS)
debug: $(EXENAME) $(TEXENAME)
	@./$(TEXENAME)
	@echo "Done building debug!"

release: OPTIONS := $(OPTIONS) $(RELEASEOPTIONS)
release: $(EXENAME) $(TEXENAME)
	@./$(TEXENAME)
	@echo "Done building release!"


clean:
	$(RM) $(OBJDIRPREFIX) $(DEPDIRPREFIX) $(EXENAME) $(TEXENAME) $(GENERATEDSOURCES)


$(EXENAME): $(OBJS)
	$(CC) -o $(EXENAME) $(OPTIONS) $(OBJS) $(LIBS)

$(OBJS): $$(patsubst $(OBJDIR)/%.o,$(SRCDIR)/%.c,$$@) $$(patsubst $(OBJDIR)/%.o,$(DEPDIR)/%.dep,$$@) | $$(dir $$@)
	@clang-format -i $(filter-out %.dep,$^)
	$(CC) $(OPTIONS) -c $< -o $@

$(SRCDIR)/parser/lex.yy.c: $(SRCDIR)/parser/lexer.l $(SRCDIR)/parser/parser.tab.h
	$(LEX) $(LEXOPTIONS) $(SRCDIR)/parser/lexer.l
	@$(MV) lex.yy.c src/main/parser

$(SRCDIR)/parser/parser.tab.c $(SRCDIR)/parser/parser.tab.h: $(SRCDIR)/parser/parser.y
	$(YACC) $(YACCOPTIONS) src/main/parser/parser.y
	@$(MV) parser.tab.[ch] src/main/parser

$(DEPS): $$(patsubst $(DEPDIR)/%.dep,$(SRCDIR)/%.c,$$@) $(GENERATEDSOURCES) | $$(dir $$@)
	@set -e; $(RM) $@; \
	 $(CC) $(OPTIONS) -MM -MT $(patsubst $(DEPDIR)/%.dep,$(OBJDIR)/%.o,$@) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$


$(TEXENAME): $(TOBJS) $(OBJS) $(GENERATEDOBJS)
	$(CC) -o $(TEXENAME) $(OPTIONS) $(TOPTIONS) $(filter-out %main.o,$(OBJS)) $(GENERATEDOBJS) $(TOBJS) $(LIBS)

$(TOBJS): $$(patsubst $(TOBJDIR)/%.o,$(TSRCDIR)/%.c,$$@) $$(patsubst $(TOBJDIR)/%.o,$(TDEPDIR)/%.dep,$$@) | $$(dir $$@)
	@clang-format -i $(filter-out %.dep,$^)
	$(CC) $(OPTIONS) $(TOPTIONS) -c $< -o $@

$(TDEPS): $$(patsubst $(TDEPDIR)/%.dep,$(TSRCDIR)/%.c,$$@) | $$(dir $$@)
	@set -e; $(RM) $@; \
	 $(CC) $(OPTIONS) $(TOPTIONS) -MM -MT $(patsubst $(TDEPDIR)/%.dep,$(TOBJDIR)/%.o,$@) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$


%/:
	@$(MKDIR) $@


-include $(DEPS) $(TDEPS)