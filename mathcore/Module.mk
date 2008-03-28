# Module.mk for mathcore module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 20/6/2005

MODDIR       := mathcore
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

MATHCOREDIR  := $(MODDIR)
MATHCOREDIRS := $(MATHCOREDIR)/src
MATHCOREDIRI := $(MATHCOREDIR)/inc

##### libMathCore #####
MATHCOREL1   := $(MODDIRI)/LinkDef.h
MATHCOREL2   := $(MODDIRI)/Math/LinkDef.h
MATHCORELINC := $(MODDIRI)/Math/LinkDef_Func.h
MATHCOREL3   := $(MODDIRI)/Fit/LinkDef.h
MATHCOREDS1  := $(MODDIRS)/G__Math.cxx
MATHCOREDS2  := $(MODDIRS)/G__MathCore.cxx
MATHCOREDS3  := $(MODDIRS)/G__MathFit.cxx
MATHCOREDO1  := $(MATHCOREDS1:.cxx=.o)
MATHCOREDO2  := $(MATHCOREDS2:.cxx=.o)
MATHCOREDO3  := $(MATHCOREDS3:.cxx=.o)
MATHCOREL    := $(MATHCOREL1) $(MATHCOREL2) $(MATHCOREL3)
MATHCOREDS   := $(MATHCOREDS1) $(MATHCOREDS2) $(MATHCOREDS3)
MATHCOREDO   := $(MATHCOREDO1) $(MATHCOREDO2) $(MATHCOREDO3)
MATHCOREDH   := $(MATHCOREDS:.cxx=.h)

MATHCOREDH1  := $(MODDIRI)/TComplex.h \
                $(MODDIRI)/TMath.h \
                $(MODDIRI)/TRandom.h \
                $(MODDIRI)/TRandom1.h \
                $(MODDIRI)/TRandom2.h \
                $(MODDIRI)/TRandom3.h
MATHCOREDH2  := $(MODDIRI)/Math/SpecFuncMathCore.h \
                $(MODDIRI)/Math/DistFuncMathCore.h \
                $(MODDIRI)/Math/IParamFunction.h \
                $(MODDIRI)/Math/IFunction.h \
                $(MODDIRI)/Math/Functor.h \
                $(MODDIRI)/Math/Minimizer.h \
                $(MODDIRI)/Math/Integrator.h \
                $(MODDIRI)/Math/VirtualIntegrator.h \
                $(MODDIRI)/Math/AllIntegrationTypes.h \
                $(MODDIRI)/Math/AdaptiveIntegratorMultiDim.h \
                $(MODDIRI)/Math/IntegratorMultiDim.h \
                $(MODDIRI)/Math/Factory.h \
                $(MODDIRI)/Math/FitMethodFunction.h \
                $(MODDIRI)/Math/GaussIntegrator.h \
                $(MODDIRI)/Math/GaussLegendreIntegrator.h \
                $(MODDIRI)/Math/RootFinder.h \
                $(MODDIRI)/Math/IRootFinderMethod.h \
                $(MODDIRI)/Math/RichardsonDerivator.h \
                $(MODDIRI)/Math/BrentMethods.h \
                $(MODDIRI)/Math/BrentMinimizer1D.h \
                $(MODDIRI)/Math/BrentRootFinder.h
MATHCOREDH3  := $(filter-out $(MODDIRI)/Fit/Chi2Grad%,$(wildcard $(MODDIRI)/Fit/*.h))
MATHCOREDH3  := $(filter-out $(MODDIRI)/Fit/LinkDef%,$(MATHCOREDH3))

MATHCOREH1   := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
MATHCOREH2   := $(filter-out $(MODDIRI)/Math/LinkDef%,$(wildcard $(MODDIRI)/Math/*.h))
MATHCOREH3   := $(filter-out $(MODDIRI)/Fit/LinkDef%,$(wildcard $(MODDIRI)/Fit/*.h))
MATHCOREH    := $(MATHCOREH1) $(MATHCOREH2) $(MATHCOREH3)

MATHCORES    := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
MATHCOREO    := $(MATHCORES:.cxx=.o)

MATHCOREDEP  := $(MATHCOREO:.o=.d) $(MATHCOREDO:.o=.d)

MATHCORELIB  := $(LPATH)/libMathCore.$(SOEXT)
MATHCOREMAP  := $(MATHCORELIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS      += $(patsubst $(MODDIRI)/%.h,include/%.h,$(MATHCOREH))
ALLLIBS      += $(MATHCORELIB)
ALLMAPS      += $(MATHCOREMAP)

# include all dependency files
INCLUDEFILES += $(MATHCOREDEP)

##### local rules #####
include/Math/%.h: $(MATHCOREDIRI)/Math/%.h
		@(if [ ! -d "include/Math" ]; then    \
		   mkdir -p include/Math;             \
		fi)
		cp $< $@

include/Fit/%.h: $(MATHCOREDIRI)/Fit/%.h
		@(if [ ! -d "include/Fit" ]; then     \
		   mkdir -p include/Fit;              \
		fi)
		cp $< $@

include/%.h:    $(MATHCOREDIRI)/%.h
		cp $< $@

$(MATHCORELIB): $(MATHCOREO) $(MATHCOREDO) $(ORDER_) $(MAINLIBS)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)"  \
		   "$(SOFLAGS)" libMathCore.$(SOEXT) $@     \
		   "$(MATHCOREO) $(MATHCOREDO)" \
		   "$(MATHCORELIBEXTRA)"

$(MATHCOREDS1): $(MATHCOREDH1) $(MATHCOREL1) $(ROOTCINTTMPDEP)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(MATHCOREDH1) $(MATHCOREL1)

$(MATHCOREDS2): $(MATHCOREDH2) $(MATHCOREL2) $(MATHCORELINC) $(ROOTCINTTMPDEP)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(MATHCOREDH2) $(MATHCOREL2)

$(MATHCOREDS3): $(MATHCOREDH3) $(MATHCOREL3) $(ROOTCINTTMPDEP)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(MATHCOREDH3) $(MATHCOREL3)

$(MATHCOREMAP): $(RLIBMAP) $(MAKEFILEDEP) $(ALLMATHCOREL)
		$(RLIBMAP) -o $(MATHCOREMAP) -l $(MATHCORELIB) \
		   -d $(MATHCORELIBDEPM) -c $(MATHCOREL) $(MATHCORELINC)

all-mathcore:   $(MATHCORELIB) $(MATHCOREMAP)

clean-mathcore:
		@rm -f $(MATHCOREO) $(MATHCOREDO)

clean::         clean-mathcore

distclean-mathcore: clean-mathcore
		@rm -f $(MATHCOREDEP) $(MATHCOREDS) $(MATHCOREDH) \
		   $(MATHCORELIB) $(MATHCOREMAP)
		@rm -rf include/Math include/Fit
		@cd $(MATHCOREDIR)/test; make distclean
		@cd $(MATHCOREDIR)/test/fit; make distclean

distclean::     distclean-mathcore

test-mathcore:	all-mathcore
		@cd $(MATHCOREDIR)/test; make
		@cd $(MATHCOREDIR)/test/fit; make

##### extra rules ######
$(MATHCOREO): CXXFLAGS += -DUSE_ROOT_ERROR
$(MATHCOREDO): CXXFLAGS += -DUSE_ROOT_ERROR
