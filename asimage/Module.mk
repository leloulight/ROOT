# Module.mk for asimage module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 8/8/2002

MODDIR       := asimage
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

ASIMAGEDIR   := $(MODDIR)
ASIMAGEDIRS  := $(ASIMAGEDIR)/src
ASIMAGEDIRI  := $(ASIMAGEDIR)/inc

ifeq ($(BUILTINASIMAGE), yes)
ASTEPVERS    := libAfterImage
ASTEPDIRS    := $(MODDIRS)/$(ASTEPVERS)
ASTEPDIRI    := -I$(MODDIRS)/$(ASTEPVERS)
else
ASTEPDIRI    := $(ASINCDIR:%=-I%)
ASTEPDIRS    :=
ASTEPVERS    :=
endif

##### libAfterImage #####
ifeq ($(BUILTINASIMAGE), yes)
ifeq ($(PLATFORM),win32)
ASTEPLIBA    := $(ASTEPDIRS)/libAfterImage.lib
ASTEPLIB     := $(LPATH)/libAfterImage.lib
else
ASTEPLIBA    := $(ASTEPDIRS)/libAfterImage.a
ASTEPLIB     := $(LPATH)/libAfterImage.a
endif
ASTEPDEP     := $(ASTEPLIB)
else
ASTEPLIBA    := $(ASLIBDIR) $(ASLIB)
ASTEPLIB     := $(ASLIBDIR) $(ASLIB)
ASTEPDEP     :=
endif

##### libASImage #####
ASIMAGEL     := $(MODDIRI)/LinkDef.h
ASIMAGEDS    := $(MODDIRS)/G__ASImage.cxx
ASIMAGEDO    := $(ASIMAGEDS:.cxx=.o)
ASIMAGEDH    := $(ASIMAGEDS:.cxx=.h)

ASIMAGEH     := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
ASIMAGES     := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
ASIMAGEO     := $(ASIMAGES:.cxx=.o)

ASIMAGEDEP   := $(ASIMAGEO:.o=.d) $(ASIMAGEDO:.o=.d)

ASIMAGELIB   := $(LPATH)/libASImage.$(SOEXT)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(ASIMAGEH))
ALLLIBS     += $(ASIMAGELIB)

# include all dependency files
INCLUDEFILES += $(ASIMAGEDEP)

##### local rules #####
include/%.h:    $(ASIMAGEDIRI)/%.h
		cp $< $@

ifeq ($(BUILTINASIMAGE), yes)
$(ASTEPLIB):    $(ASTEPLIBA)
		cp $< $@
		@(if [ $(PLATFORM) = "macosx" ]; then \
			ranlib $@; \
		fi)

$(ASTEPLIBA):   $(ASTEPDIRS).tar.gz
ifeq ($(PLATFORM),win32)
		@(if [ -d $(ASTEPDIRS) ]; then \
			rm -rf $(ASTEPDIRS); \
		fi; \
		echo "*** Building $@..."; \
		cd $(ASIMAGEDIRS); \
		if [ ! -d $(ASTEPVERS) ]; then \
			gunzip -c $(ASTEPVERS).tar.gz | tar xf -; \
		fi; \
		cd $(ASTEPVERS); \
		unset MAKEFLAGS; \
		nmake FREETYPEDIRI=-I../../../$(FREETYPEDIRI) -nologo -f libAfterImage.mak \
		CFG="libAfterImage - Win32 Release")
else
		@(if [ -d $(ASTEPDIRS) ]; then \
			rm -rf $(ASTEPDIRS); \
		fi; \
		echo "*** Building $@..."; \
		cd $(ASIMAGEDIRS); \
		if [ ! -d $(ASTEPVERS) ]; then \
			if [ "x`which gtar 2>/dev/null | awk '{if ($$1~/gtar/) print $$1;}'`" != "x" ]; then \
				gtar zxf $(ASTEPVERS).tar.gz; \
			else \
				gunzip -c $(ASTEPVERS).tar.gz | tar xf -; \
			fi; \
		fi; \
		cd $(ASTEPVERS); \
		ACC=; \
		ACFLAGS="-O"; \
		if [ "$(CC)" = "icc" ]; then \
			ACC="icc"; \
		fi; \
		if [ "$(ARCH)" = "sgicc64" ]; then \
			ACC="gcc -mabi=64"; \
		fi; \
		if [ "$(ARCH)" = "hpuxia64acc" ]; then \
			ACC="cc +DD64 -Ae"; \
			ACCALT="gcc -mlp64"; \
		fi; \
		if [ "$(ARCH)" = "linuxx8664gcc" ]; then \
			MMX="--enable-mmx-optimization=no"; \
		fi; \
		if [ "$(ASJPEGINCDIR)" != "" ]; then \
			JPEGINCDIR="--with-jpeg-includes=$(ASJPEGINCDIR)"; \
		fi; \
		if [ "$(ASPNGINCDIR)" != "" ]; then \
			PNGINCDIR="--with-png-includes=$(ASPNGINCDIR)"; \
		fi; \
		if [ "$(ASTIFFINCDIR)" != "" ]; then \
			TIFFINCDIR="--with-tiff-includes=$(ASTIFFINCDIR)"; \
		fi; \
		if [ "$(ASGIFINCDIR)" != "" ]; then \
			GIFINCDIR="--with-gif-includes=$(ASGIFINCDIR)"; \
			NOUNGIF="--with-ungif --with-builtin-ungif=no"; \
		else \
			NOUNGIF="--with-builtin-ungif"; \
		fi; \
		GNUMAKE=$(MAKE) CC=$$ACC CFLAGS=$$ACFLAGS \
		./configure \
		--with-ttf --with-ttf-includes=../../../$(FREETYPEDIRI) \
		--with-afterbase=no \
		--disable-glx \
		$$MMX \
		--with-builtin-ungif \
		$$JPEGINCDIR \
		$$PNGINCDIR \
		$$TIFFINCDIR \
		$$GIFINCDIR; \
		$(MAKE))
endif
endif

$(ASIMAGELIB):  $(ASIMAGEO) $(ASIMAGEDO) $(ASTEPDEP) $(FREETYPELIB) \
                $(MAINLIBS) $(ASIMAGELIBDEP)
ifeq ($(PLATFORM),win32)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libASImage.$(SOEXT) $@ \
		   "$(ASIMAGEO) $(ASIMAGEDO)" \
		   "$(ASIMAGELIBEXTRA) $(ASTEPLIB) $(ASEXTRALIBDIR) $(ASEXTRALIB) $(FREETYPELDFLAGS) $(FREETYPELIB)"
else
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libASImage.$(SOEXT) $@ \
		   "$(ASIMAGEO) $(ASIMAGEDO)" \
		   "$(ASIMAGELIBEXTRA) $(ASTEPLIB) $(ASEXTRALIBDIR) $(ASEXTRALIB) $(XLIBS) $(FREETYPELDFLAGS) $(FREETYPELIB)"
endif

$(ASIMAGEDS):   $(ASIMAGEH) $(ASIMAGEL) $(ROOTCINTTMP)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(ASIMAGEH) $(ASIMAGEL)

$(ASIMAGEDO):   $(ASIMAGEDS) $(ASTEPLIB)
		$(CXX) $(NOOPT) $(CXXFLAGS) $(ASTEPDIRI) -I. -o $@ -c $<

all-asimage:    $(ASIMAGELIB)

map-asimage:    $(RLIBMAP)
		$(RLIBMAP) -r $(ROOTMAP) -l $(ASIMAGELIB) \
		   -d $(ASIMAGELIBDEP) -c $(ASIMAGEL)

map::           map-asimage

clean-asimage:
		@rm -f $(ASIMAGEO) $(ASIMAGEDO)
ifeq ($(BUILTINASIMAGE), yes)
ifeq ($(PLATFORM),win32)
		-@(if [ -d $(ASTEPDIRS) ]; then \
			cd $(ASTEPDIRS); \
			unset MAKEFLAGS; \
			nmake -nologo -f libAfterImage.mak clean \
			CFG="libAfterImage - Win32 Release"; \
		fi)
else
		-@(if [ -d $(ASTEPDIRS) ]; then \
			cd $(ASTEPDIRS); \
			$(MAKE) clean; \
		fi)
endif
endif

clean::         clean-asimage

distclean-asimage: clean-asimage
		@rm -f $(ASIMAGEDEP) $(ASIMAGEDS) $(ASIMAGEDH) $(ASIMAGELIB)
ifeq ($(BUILTINASIMAGE), yes)
		@rm -rf $(ASTEPLIB)
endif
		@rm -rf $(ASTEPDIRS)

distclean::     distclean-asimage

##### extra rules ######
$(ASIMAGEO): %.o: %.cxx $(ASTEPLIB)
	$(CXX) $(OPT) $(CXXFLAGS) $(ASTEPDIRI) -o $@ -c $<
