# Module.mk for html module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := html
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

HTMLDIR      := $(MODDIR)
HTMLDIRS     := $(HTMLDIR)/src
HTMLDIRI     := $(HTMLDIR)/inc

##### libHtml #####
HTMLL        := $(MODDIRI)/LinkDef.h
HTMLDS       := $(MODDIRS)/G__Html.cxx
HTMLDO       := $(HTMLDS:.cxx=.o)
HTMLDH       := $(HTMLDS:.cxx=.h)

HTMLH        := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
HTMLS        := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
HTMLO        := $(HTMLS:.cxx=.o)

HTMLDEP      := $(HTMLO:.o=.d) $(HTMLDO:.o=.d)

HTMLLIB      := $(LPATH)/libHtml.$(SOEXT)
HTMLMAP      := $(HTMLLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(HTMLH))
ALLLIBS     += $(HTMLLIB)
ALLMAPS     += $(HTMLMAP)

# include all dependency files
INCLUDEFILES += $(HTMLDEP)

##### local rules #####
include/%.h:    $(HTMLDIRI)/%.h
		cp $< $@

$(HTMLLIB):     $(HTMLO) $(HTMLDO) $(ORDER_) $(MAINLIBS) $(HTMLLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libHtml.$(SOEXT) $@ "$(HTMLO) $(HTMLDO)" \
		   "$(HTMLLIBEXTRA)"

$(HTMLDS):      $(HTMLH) $(HTMLL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(HTMLH) $(HTMLL)

$(HTMLMAP):     $(RLIBMAP) $(MAKEFILEDEP) $(HTMLL)
		$(RLIBMAP) -o $(HTMLMAP) -l $(HTMLLIB) \
		   -d $(HTMLLIBDEPM) -c $(HTMLL)

all-html:       $(HTMLLIB) $(HTMLMAP)

clean-html:
		@rm -f $(HTMLO) $(HTMLDO)

clean::         clean-html

distclean-html: clean-html
		@rm -f $(HTMLDEP) $(HTMLDS) $(HTMLDH) $(HTMLLIB) $(HTMLMAP)

distclean::     distclean-html
