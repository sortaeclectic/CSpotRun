rem For versions 1.0.5 and earlier, there is a tag for each version.
rem I haven't copied all these into the cases below. See 1.0.5 
rem for an example if you need earlier versions.

goto 1.0.5dev1

:1.0.5dev1
	set VERSION=1.0.5dev1
	set CVSARGS=-D "Sun Jun 17 17:24:13 2001 UTC"
	set DOSABLEVERSION=1p0p5dev1
	goto gogogo
	
:1.0.5
	set VERSION=1.0.5
	set CVSARGS=-r CSPOTRUN_1_0_5
	set DOSABLEVERSION=1p0p5
	goto gogogo

:gogogo

echo building version %VERSION% (%DOSABLEVERSION%) 

set SRCZIP=CSpotRunSrc%DOSABLEVERSION%.zip	
set TEMPDRIVE=u:
set TEMPDIR=%TEMPDRIVE%\tmp
set OUTPUTDIR=%TEMPDIR%\%DOSABLEVERSION%\output
set WEBDRIVE=c:
set WEBDIR=%WEBDRIVE%\webroot\clagett\bill\palmos\cspotrun\%DOSABLEVERSION%
