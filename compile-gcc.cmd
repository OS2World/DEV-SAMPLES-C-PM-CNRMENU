@echo off
set EMXOMFLD_TYPE=WLINK
set EMXOMFLD_LINKER=wl.exe
set EMXOMFLD_PRELINK=0
if not exist bin-gcc mkdir bin-gcc
make -f makefile-gcc %1 2>&1 | tee compile-gcc.log
