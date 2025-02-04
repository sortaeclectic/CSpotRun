# Makefile for CSpotRun
#
# On Windows, I have to set MAKEMODE=UNIX in my environment to make
# this work.

CC = m68k-palmos-gcc -palmos5r3
MULTILINK = m68k-palmos-multilink
ZIP = zip
PILRC = pilrc -q
BUILDPRC = build-prc
MULTIGEN = m68k-palmos-multigen
OUTPUTDIR = ../output
RESDIR = ./res
GNUTURDS = ../copying.txt ../readme.txt
PROGNAME = CSpotRun

#USE_MULTILINK = yup 

ifndef VERSION
VERSION = dev 
endif

APPID = CSBR
LANGUAGES = chinese czech dutch english estonian french \
            german italian no_NO polish polish1250 polish8859-2\
            pt_BR pt_PT turkish sv_SE \
            spanish  

ICONTEXT = $(PROGNAME)

OBJS = pilotmain.o mainform.o doclist.o doc.o \
		decode.o docprefs.o tabbedtext.o controlsform.o \
		ucgui.o rotate.o prefsform.o searchform.o \
		bmk.o bmknamefrm.o bmkedfrm.o \
		cspotrun-sections.o hyphenate.o

STDDEP = Makefile resources.h app.h appstate.h callback.h

CSR_OPTIONS = 	\
	ENABLE_ROTATION \
	ENABLE_SEARCH \
	ENABLE_AUTOSCROLL \
	ENABLE_BMK \
	ENABLE_HYPHEN \
	SUPER0 

DEFINES = \
	-DAPPID=$(APPID) \
	-DappId=\'$(APPID)\' \
	-DERROR_CHECK_LEVEL=ERROR_CHECK_FULL \
	$(foreach opt, $(CSR_OPTIONS), -D$(opt)) \
	-DNON_INTERNATIONAL 

LIBS = -lPalmOSGlue
ifdef DEBUG
CFLAGS = -g $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch 
else
CFLAGS = -O2 $(DEFINES) $(INCLUDES) -Wall -W -Wno-unused -Wno-switch 
endif

RCP_IFDEFFER = awk '\
	BEGIN { \
		split("$(CSR_OPTIONS)", tmp, " "); \
		for (s in tmp) defined[tmp[s]]=""; d=0\
	}\
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
	$(ZIP) -j $@ $^ $(GNUTURDS) \
                $(wildcard hyphen_db/CSpotRun_hyphen_$*.pdb)

ifdef USE_MULTILINK

$(PROGNAME): $(OBJS) 
	$(MULTILINK) -fid $(APPID) \
		$(OBJS) \
		/usr/local/palmdev/sdk/lib/m68k-palmos-coff/libPalmOSGlue.a \

$(PROGNAME)_%.prc: %.resstamp $(PROGNAME) 
	$(BUILDPRC) $(PROGNAME) $(APPID) *.bin *.grc

else

$(PROGNAME): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@ cspotrun-sections.ld

$(PROGNAME)_%.prc: cspotrun.def %.resstamp $(PROGNAME) 
	$(BUILDPRC) cspotrun.def -o $@ $(PROGNAME) *.bin 

endif

%.resstamp: $(RESDIR)/fixed_resources.rcp fixed_resources.h $(RESDIR)/*.bmp 
	$(PILRC) -I $(RESDIR) -q -L $* $(RESDIR)/fixed_resources.rcp  > /dev/null; 
	touch $@

$(RESDIR)/resources.rcp: $(RESDIR)/resources_neutral.rcp $(RCPS)
	cat $(RCPS) $(RESDIR)/resources_neutral.rcp > $@

$(RESDIR)/fixed_resources.rcp: $(RESDIR)/resources.rcp
	cat $^ | sed -e "s/@@CSR_VERSION@@/$(VERSION)/g" | $(RCP_IFDEFFER)  > $@

fixed_resources.h: resources.h
	$(CC) $(DEFINES) -E -dD resources.h > $@

.o: 
	$(CC) $(CFLAGS) -o $@

cspotrun-sections.o: cspotrun-sections.s
	$(CC) -c cspotrun-sections.s

cspotrun-sections.s cspotrun-sections.ld: cspotrun.def
	$(MULTIGEN) cspotrun.def

resources.zip:
	$(ZIP) $@ $(RCPS) 

clean:
	rm -rf *.[oa] $(PROGNAME) *.prs *.bin *.stamp *.resstamp *.grc $(PRCS) fixed_resources.h $(RESDIR)/fixed_resources.rcp $(RESDIR)/resources.rcp $(LANGUAGES) $(ZIPS) *.s *.ld

depend:
	makedepend -Y -- $(CFLAGS) -- *.c

# DO NOT DELETE


bmkedfrm.o: bmkedfrm.h bmk.h bmknamefrm.h callback.h resources.h segments.h
bmknamefrm.o: bmk.h bmknamefrm.h callback.h resources.h segments.h
bmk.o: appstate.h bmk.h doc.h docprefs.h resources.h rotate.h segments.h
controlsform.o: app.h appstate.h callback.h controlsform.h docprefs.h
controlsform.o: mainform.h resources.h segments.h ucgui.h
decode.o: csrdrmlib.h decode.h segments.h
doclist.o: app.h appstate.h doc.h doclist.h docprefs.h rotate.h segments.h
doc.o: appstate.h bmk.h decode.h doc.h docprefs.h hyphenate.h mainform.h
doc.o: resources.h rotate.h segments.h tabbedtext.h
docprefs.o: appstate.h doclist.h docprefs.h rotate.h segments.h
hyphenate.o: hyphenate.h segments.h
mainform.o: app.h appstate.h bmk.h bmknamefrm.h callback.h doc.h doclist.h
mainform.o: docprefs.h mainform.h resources.h rotate.h segments.h sonyclie.h
mainform.o: ucgui.h
pilotmain.o: app.h appstate.h bmkedfrm.h bmk.h bmknamefrm.h controlsform.h
pilotmain.o: decode.h docprefs.h hyphenate.h mainform.h prefsform.h
pilotmain.o: resources.h searchform.h segments.h tabbedtext.h ucgui.h
prefsform.o: app.h appstate.h callback.h controlsform.h doc.h docprefs.h
prefsform.o: mainform.h resources.h rotate.h segments.h ucgui.h
rotate.o: app.h rotate.h segments.h
searchform.o: app.h appstate.h callback.h doc.h docprefs.h mainform.h
searchform.o: resources.h rotate.h searchform.h segments.h
tabbedtext.o: segments.h tabbedtext.h
ucgui.o: appstate.h docprefs.h resources.h rotate.h segments.h ucgui.h
