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
		fontselect.o ucgui.o rotate.o prefsform.o searchform.o \
		bmk.o bmknamefrm.o bmkedfrm.o 

STDDEP = Makefile resources.h app.h appstate.h callback.h

CSR_OPTIONS = DEBUG ENABLE_ROTATION ENABLE_SEARCH ENABLE_AUTOSCROLL ENABLE_BMK
DEFINES = -DAPPID=$(APPID) -DappId=\'$(APPID)\' -DERROR_CHECK_LEVEL=ERROR_CHECK_FULL $(foreach opt, $(CSR_OPTIONS), -D$(opt)) -DNON_INTERNATIONAL 

ifdef DEBUG
CFLAGS = -g $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch 
else
CFLAGS = -O2 $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch 
endif

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
zips: $(ZIPS) resources.zip 

release: $(ZIPS) $(PRCS)
	cp $(ZIPS) $(PRCS) $(OUTPUTDIR)

$(PROGNAME)_%.zip: $(PROGNAME)_%.prc
	$(ZIP) $@ $^ $(GNUTURDS); 

$(PROGNAME)_%.prc: %.resstamp code.stamp
	$(BUILDPRC) $@ $(ICONTEXT) $(APPID) *.grc *.bin ;

%.resstamp: $(RESDIR)/fixed_resources.rcp fixed_resources.h $(RESDIR)/*.bmp 
	$(PILRC) -I $(RESDIR) -q -L $* $(RESDIR)/fixed_resources.rcp  > /dev/null; 
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

resources.zip:
	$(ZIP) $@ $(RCPS) 

clean:
	rm -rf *.[oa] $(PROGNAME) *.prs *.bin *.stamp *.resstamp *.grc $(PRCS) fixed_resources.h $(RESDIR)/fixed_resources.rcp $(RESDIR)/resources.rcp $(LANGUAGES) $(ZIPS) 

depend:
	makedepend -Y -- $(CFLAGS) -- *.c

# DO NOT DELETE


bmkedfrm.o: bmkedfrm.h bmk.h bmknamefrm.h callback.h resources.h
bmknamefrm.o: bmk.h bmknamefrm.h callback.h resources.h
bmk.o: appstate.h bmk.h doc.h docprefs.h resources.h rotate.h
controlsform.o: app.h appstate.h callback.h controlsform.h docprefs.h
controlsform.o: mainform.h resources.h ucgui.h
decode.o: decode.h
doclist.o: app.h appstate.h doc.h doclist.h docprefs.h rotate.h
doc.o: appstate.h bmk.h decode.h doc.h docprefs.h mainform.h resources.h
doc.o: rotate.h tabbedtext.h
docprefs.o: appstate.h doclist.h docprefs.h rotate.h
fontselect.o: appstate.h doc.h docprefs.h fontselect.h resources.h rotate.h
mainform.o: app.h appstate.h bmk.h bmknamefrm.h callback.h doc.h doclist.h
mainform.o: docprefs.h fontselect.h mainform.h resources.h rotate.h ucgui.h
pilotmain.o: app.h appstate.h bmkedfrm.h bmk.h bmknamefrm.h
pilotmain.o: controlsform.h docprefs.h mainform.h prefsform.h resources.h
pilotmain.o: searchform.h ucgui.h
prefsform.o: app.h appstate.h callback.h controlsform.h doc.h docprefs.h
prefsform.o: mainform.h resources.h rotate.h ucgui.h
rotate.o: app.h rotate.h
searchform.o: app.h appstate.h callback.h doc.h docprefs.h mainform.h
searchform.o: resources.h rotate.h searchform.h
tabbedtext.o: tabbedtext.h
ucgui.o: appstate.h docprefs.h fontselect.h resources.h ucgui.h
