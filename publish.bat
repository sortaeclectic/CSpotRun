@echo off

pushd .

call buildenv.bat

mkdir %WEBDIR%
copy %OUTPUTDIR%\* %WEBDIR%\.

popd
