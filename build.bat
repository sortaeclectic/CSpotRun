set V1=0
set V2=9
set V3=6

set TAG=CSPOTRUN_%V1%_%V2%_%V3%
set SRCZIP=CSpotRunSrc%V1%p%V2%p%V3%.zip
set VERSION=%V1%.%V2%.%V3%

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
