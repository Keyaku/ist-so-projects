SRCDIR ?= src
OBJDIR = bld
DEPS = $(OBJDIR)/*.d

INCLUDE += -I$(SRCDIR)

CC = gcc
LD = $(CC)
CFLAGS = -O0 -g -c -pedantic -Wall $(INCLUDE)

ifneq ($(shell uname), Darwin)
	LDFLAGS = -pthread
endif

ZIPFILE = ex1-g48.zip
EXECFILE ?= heatSim

################################################################################
#############       DO NOT CHANGE ANYTHING BELOW THIS PART        ##############
################################################################################
EXEC = $(EXECFILE)

# Filtering files for rule management
SRC := $(shell find $(SRCDIR) -name *.c)
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
	rm -rf $(ZIPFILE) $(OBJDIR)/* $(DEPS) $(EXEC) *.DS_Store

.PHONY: all clean zip
