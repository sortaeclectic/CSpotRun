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
		bmk.o bmknamefrm.o bmkedfrm.o bmksortfrm.o

STDDEP = Makefile resources.h app.h appstate.h callback.h

CSR_OPTIONS = DEBUG ENABLE_ROTATION ENABLE_SEARCH ENABLE_AUTOSCROLL ENABLE_BMK
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
zips: $(ZIPS) resources.zip 

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

resources.zip:
	$(ZIP) $@ $(RCPS) 

clean:
	rm -rf *.[oa] $(PROGNAME) *.prs *.bin *.stamp *.resstamp *.grc $(PRCS) fixed_resources.h $(RESDIR)/fixed_resources.rcp $(RESDIR)/resources.rcp $(LANGUAGES) $(ZIPS) 

depend:
	makedepend -Y -- $(CFLAGS) -- *.c 
# DO NOT DELETE

bmk.o: bmk.h appstate.h docprefs.h doc.h rotate.h resources.h
bmkedfrm.o: resources.h callback.h bmkedfrm.h bmknamefrm.h bmk.h
bmknamefrm.o: resources.h callback.h bmknamefrm.h bmk.h
bmksortfrm.o: resources.h callback.h bmksortfrm.h bmk.h
controlsform.o: resources.h callback.h app.h appstate.h docprefs.h
controlsform.o: controlsform.h ucgui.h mainform.h
decode.o: decode.h
doc.o: appstate.h docprefs.h rotate.h doc.h resources.h mainform.h decode.h
doc.o: tabbedtext.h bmk.h
doclist.o: app.h doclist.h rotate.h appstate.h docprefs.h doc.h
docprefs.o: doclist.h rotate.h docprefs.h appstate.h
fontselect.o: resources.h appstate.h docprefs.h rotate.h fontselect.h doc.h
mainform.o: resources.h mainform.h callback.h app.h doclist.h rotate.h
mainform.o: appstate.h docprefs.h doc.h ucgui.h fontselect.h bmk.h
pilotmain.o: app.h resources.h mainform.h controlsform.h bmknamefrm.h
pilotmain.o: bmkedfrm.h bmksortfrm.h bmk.h appstate.h docprefs.h ucgui.h
pilotmain.o: prefsform.h searchform.h
prefsform.o: resources.h callback.h app.h appstate.h docprefs.h
prefsform.o: controlsform.h ucgui.h mainform.h doc.h rotate.h
rotate.o: rotate.h app.h
searchform.o: resources.h callback.h app.h appstate.h docprefs.h searchform.h
searchform.o: mainform.h doc.h rotate.h
tabbedtext.o: tabbedtext.h
ucgui.o: resources.h ucgui.h appstate.h docprefs.h fontselect.h
