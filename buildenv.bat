set V1=1
set V2=0
set V3=5

rem ### TAG is the CVS tag.
set TAG=CSPOTRUN_%V1%_%V2%
if %V3% neq 0 set TAG=%TAG%_%V3%

rem ### SRCZIP is the name of the zipped source archive.
set SRCZIP=CSpotRunSrc%V1%p%V2%
if %V3% neq 0 set SRCZIP=%SRCZIP%p%V3%
set SRCZIP=%SRCZIP%.zip

rem ### VERSION is the version string
set VERSION=%V1%.%V2%
if %V3% neq 0 set VERSION=%VERSION%.%V3%

set TEMPDRIVE=u:
set TEMPDIR=%TEMPDRIVE%\tmp
set OUTPUTDIR=%TEMPDIR%\%TAG%\output

set WEBDRIVE=c:
set WEBDIR=%WEBDRIVE%\webroot\clagett\bill\palmos\cspotrun\%V1%p%V2%
if %V3% neq 0 set WEBDIR=%WEBDIR%p%V3%
