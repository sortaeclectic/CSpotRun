@echo off

call buildenv.bat

pushd .

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
cd src

rem ### COMPILE
make -s -e VERSION=%VERSION% zips

move *.zip %OUTPUTDIR%

popd

@echo 
@echo ### The goodies should be in %OUTPUTDIR%

