@echo off
if not exist bin-ow mkdir bin-ow
wmake -f makefile-ow %1 2>&1 | tee compile-ow.log
