set TAG=CSPOTRUN_0_9_4
set SRCZIP=CSpotRunSrc0p9p4.zip
set VERSION=0.9.4

#set TAG=CSPOTRUN_0_9_3
#set SRCZIP=CSpotRunSrc0p9p3.zip
#set VERSION=0.9.3

#set TAG=CSPOTRUN_0_9_2
#set SRCZIP=CSpotRunSrc0p9p2.zip
#set VERSION=0.9.2

#set TAG=CSPOTRUN_0_9_1
#set SRCZIP=CSpotRunSrc0p9p1.zip
#set VERSION=0.9.1

#set TAG=CSPOTRUN_0_9
#set SRCZIP=CSpotRunSrc0p9.zip
#set VERSION=0.9

#set TAG=CSPOTRUN_0_8_3
#set SRCZIP=CSpotRunSrc0p8p3.zip

set TEMPDRIVE=d:
set TEMPDIR=%TEMPDRIVE%\temp
set OUTPUTDIR=%TEMPDIR%\%TAG%\output

%TEMPDRIVE%
cd %TEMPDIR%
mkdir %TAG%
mkdir %OUTPUTDIR%
cd %TAG%

###  PULL
cvs export -r %TAG% cspotrun

### MAKE SOURCE ZIPBALL
cd cspotrun
pkzip -recurse -path=current -add %OUTPUTDIR%\%SRCZIP% src\* *.txt

cd src

### COMPILE
make -e VERSION=%VERSION% zips

move *.zip %OUTPUTDIR%
