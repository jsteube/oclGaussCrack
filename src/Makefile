## Simply clone this from https://github.com/KhronosGroup/OpenCL-Headers and add a symlink inside: ln -s . CL

OCL             := OpenCL-Headers

##
## Compiler paths
##

BIN             := .

CPP_LINUX_32    := gcc
CPP_LINUX_64    := gcc

CPP_WIN_32      := i686-w64-mingw32-gcc
CPP_WIN_64      := x86_64-w64-mingw32-gcc

DLL_WIN_32      := /usr/bin/i686-w64-mingw32-dlltool
DLL_WIN_64      := /usr/bin/x86_64-w64-mingw32-dlltool

##
## Compiler flags
##

CFLAGS          := -O2 -s -pipe -W -Wall -pedantic -std=c99 -I$(OCL)

CFLAGS_32       := -m32
CFLAGS_64       := -m64

CFLAGS_LINUX    := -DLINUX
CFLAGS_WIN      := -DWIN   -D__MSVCRT__ -D__USE_MINGW_ANSI_STDIO=1

LFLAGS_LINUX    :=
LFLAGS_WIN      := -static-libgcc -L. -lpsapi

##
## Targets
##

all: linux win

clean:
	rm -f $(BIN)/*.bin $(BIN)/*.exe $(BIN)/*.a

linux:  oclGaussCrack32.bin oclGaussCrack64.bin gaussCombinator32.bin gaussCombinator64.bin gaussFilter32.bin gaussFilter64.bin
win:    libOpenCL.a libOpenCL64.a oclGaussCrack32.exe oclGaussCrack64.exe gaussCombinator32.exe gaussCombinator64.exe gaussFilter32.exe gaussFilter64.exe

libOpenCL.a:
	${DLL_WIN_32} -A -k -l libOpenCL.a   -d lib/OpenCL.def

libOpenCL64.a:
	${DLL_WIN_64} -A -k -l libOpenCL64.a -d lib/OpenCL64.def

oclGaussCrack32.bin: src/oclGaussCrack.c
	$(CPP_LINUX_32) $(CFLAGS) $(CFLAGS_32) $(CFLAGS_LINUX) -o $@ $^ $(LFLAGS_LINUX) -lOpenCL

oclGaussCrack64.bin: src/oclGaussCrack.c
	$(CPP_LINUX_64) $(CFLAGS) $(CFLAGS_64) $(CFLAGS_LINUX) -o $@ $^ $(LFLAGS_LINUX) -lOpenCL

oclGaussCrack32.exe: src/oclGaussCrack.c
	$(CPP_WIN_32)   $(CFLAGS) $(CFLAGS_32) $(CFLAGS_WIN)   -o $@ $^ $(LFLAGS_WIN)   -lOpenCL

oclGaussCrack64.exe: src/oclGaussCrack.c
	$(CPP_WIN_64)   $(CFLAGS) $(CFLAGS_64) $(CFLAGS_WIN)   -o $@ $^ $(LFLAGS_WIN)   -lOpenCL64

gaussCombinator32.bin: src/gaussCombinator.c
	$(CPP_LINUX_32) $(CFLAGS) $(CFLAGS_32) $(CFLAGS_LINUX) -o $@ $^ $(LFLAGS_LINUX)

gaussCombinator64.bin: src/gaussCombinator.c
	$(CPP_LINUX_64) $(CFLAGS) $(CFLAGS_64) $(CFLAGS_LINUX) -o $@ $^ $(LFLAGS_LINUX)

gaussCombinator32.exe: src/gaussCombinator.c
	$(CPP_WIN_32)   $(CFLAGS) $(CFLAGS_32) $(CFLAGS_WIN)   -o $@ $^ $(LFLAGS_WIN)

gaussCombinator64.exe: src/gaussCombinator.c
	$(CPP_WIN_64)   $(CFLAGS) $(CFLAGS_64) $(CFLAGS_WIN)   -o $@ $^ $(LFLAGS_WIN)

gaussFilter32.bin: src/gaussFilter.c
	$(CPP_LINUX_32) $(CFLAGS) $(CFLAGS_32) $(CFLAGS_LINUX) -o $@ $^ $(LFLAGS_LINUX)

gaussFilter64.bin: src/gaussFilter.c
	$(CPP_LINUX_64) $(CFLAGS) $(CFLAGS_64) $(CFLAGS_LINUX) -o $@ $^ $(LFLAGS_LINUX)

gaussFilter32.exe: src/gaussFilter.c
	$(CPP_WIN_32)   $(CFLAGS) $(CFLAGS_32) $(CFLAGS_WIN)   -o $@ $^ $(LFLAGS_WIN)

gaussFilter64.exe: src/gaussFilter.c
	$(CPP_WIN_64)   $(CFLAGS) $(CFLAGS_64) $(CFLAGS_WIN)   -o $@ $^ $(LFLAGS_WIN)
