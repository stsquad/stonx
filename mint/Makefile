#
# Makefile for
#
TARGET = libmint-native.a

srcdir = .
top_srcdir = ..
subdir = mint

default: all

include $(top_srcdir)/RULES

ifeq ($(XFSSUBDIRS),stonx.xfs)
SUBDIRS = $(XFSSUBDIRS)
else
SUBDIRS =
endif

ifeq ($(top_objdir),)
  top_objdir = $(top_srcdir)
endif

# default overwrites
ifeq ($(HAVE_M68K_CROSS),yes)
ifneq ($(XFSSUBDIRS),stonx.xfs)
all: stonx.xfs all-recursive all-here

export XFSSUBDIRS = stonx.xfs
endif
endif

# default definitions
OBJS = $(addprefix $(top_objdir)/,$(COBJS:.c=.o))

all-here: $(OBJS)

distclean-here:
	$(RM) stonx.xfs
	$(RM) *~ #*
	$(RMDIR) .deps $(GENFILES)

$(OBJS): $(COBJS)
	$(CC) $(CFLAGS) $(@F:.o=.c) -o $@

ifeq ($(HAVE_M68K_CROSS),yes)
stonx.xfs:
	test -d $(FREEMiNT_SYS)/xfs/stonx \
	|| $(MKDIR) $(FREEMiNT_SYS)/xfs/stonx
	$(CPDIR) atari/* $(FREEMiNT_SYS)/xfs/stonx
	$(SYMLINK) $(FREEMiNT_SYS)/xfs/stonx stonx.xfs
	$(MAKE)
endif

include $(top_srcdir)/DEPENDENCIES

