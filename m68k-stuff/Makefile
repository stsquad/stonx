#
# Makefile for
#
TARGET = m68k_stuff_dump.o

SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = m68k-stuff

default: all

include $(top_srcdir)/RULES

ifeq ($(top_objdir),)
  top_objdir = $(top_srcdir)
endif

all-here: $(top_objdir)/$(TARGET)

# default overwrites
clean-here:
	$(RM) *.tmp
	$(RM) $(top_objdir)/$(TARGET)
	$(RM) core report errlog
	$(RM) foobar foo bar baz junk schrott muell 
	$(RM) test.c test.o test.s test

# default definitions
OBJS = $(COBJS:.c=.o)

$(top_objdir)/$(TARGET):	m68k_stuff_dump.c
	$(CC) $(CFLAGS) $< -o $@

m68k_stuff_dump.c:	m68k-stuff.o m68k-stuff.pl
	$(PERL) m68k-stuff.pl > $@

m68k-stuff.o:	m68k-stuff.s
ifeq ($(HAVE_M68K_CROSS),yes)
	$(M68K_CC) $(M68K_CFLAGS) $< -o $@
else
	@echo
	@echo '============================================================'
	@echo 'Seams you have changed or deleted m68k-stuff.s but you have '
	@echo 'set HAVE_M68K_CROSS to no. So you cannot make a new         '
	@echo 'm68k-stuff.o. This may result in fatal errors.              '
	@echo '============================================================'
	@echo
	$(TOUCH) m68k-stuff.o
endif

include $(top_srcdir)/DEPENDENCIES
