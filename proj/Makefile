SRCDIR ?= .
OBJDIR = .
DEPS = $(OBJDIR)/*.d

INCLUDE += -I$(SRCDIR)

CC = gcc
LD = $(CC)
CFLAGS = -O0 -g -c -std=gnu99 -pedantic -Wall -Wextra $(INCLUDE)
CFLAGS += -Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 \
	-Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith \
	-Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline \
	-Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
	-Wstrict-aliasing=2 -Wredundant-decls -Wold-style-definition

ifneq ($(shell uname), Darwin)
	LDFLAGS = -pthread
endif

ZIPFILE = ex4-g48.zip
EXECFILE ?= heatSim

################################################################################
#############       DO NOT CHANGE ANYTHING BELOW THIS PART        ##############
################################################################################
EXEC = $(EXECFILE)

# Filtering files for rule management
SRC := $(shell find $(SRCDIR) -name "*.c")
OBJ := $(subst $(SRCDIR),$(OBJDIR),$(SRC:%.c=%.o))

# Objects and dependencies compilation
$(OBJDIR)/%.o: $(addprefix $(SRCDIR)/,%.c)
	$(shell mkdir -p $(dir $@))
	$(CC) -MMD -o $@ $< $(CFLAGS)

# General rules
all: $(EXEC)

# Linking
$(EXEC): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# Including dependencies
-include $(DEPS)

################################################################################
#############       DO NOT CHANGE ANYTHING ABOVE THIS PART        ##############
################################################################################

zip:
	zip -v $(ZIPFILE) $(SRCDIR)/*.[ch] Makefile

clean:
	rm -rf $(ZIPFILE) $(OBJDIR)/*.o $(DEPS) $(EXEC) *.DS_Store

.PHONY: all clean zip
