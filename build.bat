@echo off
call buildenv.bat

pushd .

%TEMPDRIVE%
cd %TEMPDIR%
mkdir %DOSABLEVERSION%
mkdir %OUTPUTDIR%
cd %DOSABLEVERSION%

rem ###  PULL
cvs -Q export %CVSARGS% cspotrun

rem ### MAKE SOURCE ZIPBALL
echo making source zipball
cd cspotrun
pkzip -silent -recurse -path=current -add %OUTPUTDIR%\%SRCZIP% src\* *.txt
cd src

rem ### COMPILE
echo making
make -s -e VERSION=%VERSION% zips

echo move stuff to %OUTPUTDIR%
move *.zip %OUTPUTDIR%

popd

@echo 
@echo ### The goodies should be in %OUTPUTDIR%

