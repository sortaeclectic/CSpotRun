set TAG=CSPOTRUN_0_9
set SRCZIP=CSpotRunSrc0p9.zip

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

REM *** PULL
cvs export -r %TAG% cspotrun

REM *** MAKE SOURCE ZIPBALL
cd cspotrun
pkzip -recurse -path=current -add %OUTPUTDIR%\%SRCZIP% src\* *.txt

cd src

REM *** COMPILE
make zips

move *.zip %OUTPUTDIR%
