@echo off

pushd .

set V1=0
set V2=9
set V3=11

set TAG=CSPOTRUN_%V1%_%V2%_%V3%
set SRCZIP=CSpotRunSrc%V1%p%V2%p%V3%.zip
set VERSION=%V1%.%V2%.%V3%

set TEMPDRIVE=u:
set TEMPDIR=%TEMPDRIVE%\tmp
set OUTPUTDIR=%TEMPDIR%\%TAG%\output

%TEMPDRIVE%
cd %TEMPDIR%
mkdir %TAG%
mkdir %OUTPUTDIR%
cd %TAG%

rem ###  PULL
cvs export -r %TAG% cspotrun

rem ### MAKE SOURCE ZIPBALL
cd cspotrun
pkzip -recurse -path=current -add %OUTPUTDIR%\%SRCZIP% src\* *.txt
pkzip -recurse -path=current -add %OUTPUTDIR%\resources.zip src\res\*.rcp 
cd src

rem ### COMPILE
make -s -e VERSION=%VERSION% zips

move *.zip %OUTPUTDIR%

popd

@echo ### The goodies should be in %OUTPUTDIR%

