# Makefile for CSpotRun
#
# TODO: make this compile on some machine other than Bill's
# TODO: get some gnow-it-all to make this more gnu-like.
# TODO: dist target
#
# On Windows, I have to set MAKEMODE=UNIX in my environment to make
# this work.

CC = m68k-palmos-gcc -palmos3.1
ZIP = zip
PILRC = pilrc -q
OBJRES = m68k-palmos-obj-res
BUILDPRC = build-prc
OUTPUTDIR = ../output
RESDIR = ./res
GNUTURDS = ../copying.txt ../readme.txt
PROGNAME = CSpotRun
VERSION = dev 
APPID = CSBR
LANGUAGES = chinese czech dutch english estonian french \
            german italian polish polish1250 polish8859-2\
            pt_BR pt_PT turkish sv_SE \
            spanish  

ICONTEXT = $(PROGNAME)

OBJS = pilotmain.o mainform.o doclist.o doc.o \
		decode.o docprefs.o tabbedtext.o controlsform.o \
		fontselect.o ucgui.o rotate.o prefsform.o searchform.o

STDDEP = Makefile resources.h app.h appstate.h callback.h

CSR_OPTIONS = DEBUG ENABLE_ROTATION ENABLE_SEARCH ENABLE_AUTOSCROLL
DEFINES = -DAPPID=$(APPID) -DappId=\'$(APPID)\' -DERROR_CHECK_LEVEL=ERROR_CHECK_FULL $(foreach opt, $(CSR_OPTIONS), -D$(opt)) -DNON_INTERNATIONAL 

#CFLAGS = -g $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch 
CFLAGS = -O2 $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch 

RCP_IFDEFFER = awk 'BEGIN {split("$(CSR_OPTIONS)", tmp, " "); for (s in tmp) defined[tmp[s]]=""; d=0}\
					{\
						if ($$0 ~ "\#ifdef") \
							{stack[d]=($$2 in defined); d++;} \
						else if ($$0 ~ "\#ifndef") \
							{stack[d]=!($$2 in defined); d++;} \
						else if ($$0 ~ "\#else") \
							{stack[d-1]=!stack[d-1];} \
						else if ($$0 ~ "\#endif") \
							{d--; delete stack[d];} \
						else { \
							for (s in stack) if (!stack[s]) next;\
							print $$0;\
						}\
					}'

PRCS = $(foreach lang, $(LANGUAGES), $(PROGNAME)_$(lang).prc)
ZIPS = $(foreach lang, $(LANGUAGES), $(PROGNAME)_$(lang).zip) 
RCPS = $(foreach lang, $(LANGUAGES), $(RESDIR)/resources_$(lang).rcp) 

englishprconly: CSpotRun_english.prc
allprcs: $(PRCS) 
all: zips 
zips: $(ZIPS) 

release: $(ZIPS) $(PRCS)
	cp $(ZIPS) $(PRCS) $(OUTPUTDIR)

$(PROGNAME)_%.zip: $(PROGNAME)_%.prc
	$(ZIP) $@ $^ $(GNUTURDS); 

$(PROGNAME)_%.prc: %.resstamp code.stamp
	$(BUILDPRC) $@ $(ICONTEXT) $(APPID) *.grc *.bin ;

%.resstamp: $(RESDIR)/fixed_resources.rcp fixed_resources.h $(RESDIR)/*.bmp 
	$(PILRC) -I $(RESDIR) -q -L $* $(RESDIR)/fixed_resources.rcp  ; 
	touch $@

code.stamp: $(PROGNAME)
	$(OBJRES) $(PROGNAME) ;
	touch $@ 

$(RESDIR)/resources.rcp: $(RESDIR)/resources_neutral.rcp $(RCPS)
	cat $(RCPS) $(RESDIR)/resources_neutral.rcp > $@

$(RESDIR)/fixed_resources.rcp: $(RESDIR)/resources.rcp
	cat $^ | sed -e "s/@@CSR_VERSION@@/$(VERSION)/g" | $(RCP_IFDEFFER)  > $@

fixed_resources.h: resources.h
	$(CC) $(DEFINES) -E -dD resources.h > $@

$(PROGNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

.o: 
	$(CC) $(CFLAGS) -o $@

clean:
	rm -rf *.[oa] $(PROGNAME) *.prs *.bin *.stamp *.resstamp *.grc $(PRCS) fixed_resources.h $(RESDIR)/fixed_resources.rcp $(RESDIR)/resources.rcp $(LANGUAGES) $(ZIPS) 

pilotmain.o:    pilotmain.c $(STDDEP) mainform.h controlsform.h ucgui.h prefsform.h docprefs.h searchform.h
mainform.o:     mainform.c $(STDDEP) mainform.h doclist.h rotate.h doc.h docprefs.h ucgui.h fontselect.h
doclist.o:      doclist.c $(STDDEP) doclist.h rotate.h doc.h
doc.o:          doc.c $(STDDEP) doc.h rotate.h mainform.h decode.h docprefs.h tabbedtext.h
decode.o:       decode.c $(STDDEP) decode.h
docprefs.o:     docprefs.c $(STDDEP) docprefs.h doclist.h rotate.h
tabbedtext.o:   tabbedtext.c $(STDDEP) tabbedtext.h
controlsform.o: controlsform.c $(STDDEP) controlsform.h ucgui.h mainform.h
fontselect.o:   fontselect.c $(STDDEP) fontselect.h rotate.h doc.h
ucgui.o:        ucgui.c $(STDDEP) ucgui.h fontselect.h
rotate.o:       rotate.c $(STDDEP) rotate.h
prefsform.o:    prefsform.c $(STDDEP) prefsform.h ucgui.h mainform.h controlsform.h
searchform.o:   searchform.c  $(STDDEP) searchform.h mainform.h doc.h
