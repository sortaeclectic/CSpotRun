# Makefile for CSpotRun
#
# TODO: make this compile on some machine other than Bill's
# TODO: get some gnow-it-all to make this more gnu-like.
# TODO: dist target
#
# note: You MUST have sed and awk to compile this.
#

CC = m68k-palmos-coff-gcc
ZIP = pkzip
PILRC = pilrc -q
OBJRES = m68k-palmos-coff-obj-res
BUILDPRC = build-prc
OUTPUTDIR = ../output
RESDIR = ./res
GNUTURDS = ../copying.txt ../readme.txt


PROGNAME = CSpotRun
VERSION = 0.6
APPID = CSBR
LANGUAGES = english german spanish
ICONTEXT = $(PROGNAME)

OBJS = pilotmain.o mainform.o doclist.o doc.o \
		decode.o docprefs.o tabbedtext.o controlsform.o \
		fontselect.o ucgui.o rotate.o prefsform.o searchform.o

#CSR_OPTIONS = DEBUG ENABLE_ROTATION ENABLE_SEARCH
CSR_OPTIONS = DEBUG ENABLE_ROTATION 
DEFINES = -DAPPID=$(APPID) -DappId=\'$(APPID)\' -DERROR_CHECK_LEVEL=ERROR_CHECK_FULL $(foreach opt, $(CSR_OPTIONS), -D$(opt))
#CFLAGS =  -g $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch
CFLAGS =   -O2  $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch

RCP_IFDEFFER = awk 'BEGIN {split("$(CSR_OPTIONS)", tmp, " "); for (s in tmp) defined[tmp[s]]=""; d=0}\
					{\
						if ($$0 ~ "\#ifdef") \
							{stack[d]=$$2; d++;} \
						else if ($$0 ~ "\#endif") \
							{d--; delete stack[d];} \
						else { \
							for (s in stack) if (!(stack[s] in defined)) next;\
							print $$0;\
						}\
					}'

PRCS = $(foreach lang, $(LANGUAGES), $(PROGNAME)_$(lang).prc)
ZIPS = $(foreach lang, $(LANGUAGES), $(PROGNAME)_$(lang).zip)

englishprconly: CSpotRun_english.prc
allprcs: $(PRCS) 
all: zips 
zips: $(ZIPS) CSpotRun.zip

release: $(ZIPS) $(PRCS)
	cp $(ZIPS) $(PRCS) $(OUTPUTDIR)


CSpotRun.zip:
	$(ZIP) -add ../output/CSpotRun.zip $(PRCS) $(GNUTURDS)

$(PROGNAME)_%.zip: $(PROGNAME)_%.prc
	$(ZIP)  -add $@ $^ $(GNUTURDS); 

$(PROGNAME)_%.prc: %.resstamp code.stamp
	$(BUILDPRC) $@ $(ICONTEXT) $(APPID) *.grc *.bin ;

%.resstamp: $(RESDIR)/fixed_resources.rcp fixed_resources.h $(RESDIR)/*.bmp 
	$(PILRC) -I $(RESDIR) -q -L $* $(RESDIR)/fixed_resources.rcp  ; 
	touch $@

code.stamp: $(PROGNAME)
	$(OBJRES) $(PROGNAME) ;
	touch $@ 

$(RESDIR)/fixed_resources.rcp: $(RESDIR)/resources.rcp
	cat $^ | sed -e "s/@@CSR_VERSION@@/$(VERSION)/g" | $(RCP_IFDEFFER)  > $@

fixed_resources.h: resources.h
	$(CC) $(DEFINES) -E -dD resources.h > $@

$(PROGNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.o: 
	$(CC) $(CFLAGS) -o $@

clean:
	rm -rf *.[oa] $(PROGNAME) *.prs *.bin *.stamp *.resstamp *.grc $(PRCS) fixed_resources.h $(RESDIR)/fixed_resources.rcp $(LANGUAGES) $(ZIPS) CSpotRun.zip

