makecint -mk Makefile -dl posix.dll -h winposix.h -C winposix.c -cint -Z0
make.exe -f Makefile 
del %cintsysdir%\include\posix.dll
move posix.dll %cintsysdir%\include\posix.dll

make.exe -f Makefile clean
del Makefile
del G__*
del *.obj

