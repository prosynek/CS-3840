// os345config.h		08/27/2022
#ifndef __os345config_h__
#define __os345config_h__
// ***********************************************************************
//
#define STARTUP_MSG "CS3840 W2022"

// ***********************************************************************
// Select development system environment here:
#define DOS 0 // DOS
#define GCC 1 // UNIX/Linux
#define PPC 0 // Power PC
#define MAC 0 // Mac
#define NET 0 // NET

// ***********************************************************************
//
//	INIT_OS			Called from os345.c at startup
//	GET_CHAR		Called from pollInterrupts (os345interrupts.c);
//						returns keyboard character
//	SET_STACK(s)	Assembly instruction executed by dispatcher (os345.c);
//						sets new stack pointer
//	RESTORE_OS		Called before os exits main
//	LITTLE			Defines memory storage order (0=big endian, 1=little endian)
//	CLEAR_SCR		Called in Project 3 each second when displaying Jurassic Park
//
// ***********************************************************************
#if DOS == 1
// FOR LCC AND COMPATIBLE COMPILERS
#include <conio.h>
#define INIT_OS
#define GET_CHAR (kbhit() ? getch() : 0)
#define SET_STACK(s) _asm("movl _temp,%esp");
#define RESTORE_OS
#define LITTLE 1
#define CLEAR_SCREEN system("cls");
#endif

#if GCC == 1
// FOR GCC AND COMPATIBLE COMPILERS
#include <fcntl.h>
#define INIT_OS                                                                                                        \
    system("stty -echo -icanon");                                                                                      \
    fcntl(1, F_SETFL, O_NONBLOCK);
#define GET_CHAR getchar()
//#define SET_STACK __asm__ __volatile__("movl %0,%%esp"::"r"(temp):%esp);
#define SET_STACK(s) asm("movl temp,%esp");
#define RESTORE_OS system("stty icanon echo"); // enable canonical mode and echo
#define LITTLE 1
#define CLEAR_SCREEN system("clear");
#endif

#if NET == 1
// FOR .NET AND COMPATIBLE COMPILERS
#include <conio.h>
#define INIT_OS
#define GET_CHAR (kbhit() ? getch() : 0)
#define SET_STACK(s) __asm mov ESP, s;
#define RESTORE_OS
#define LITTLE 1
#define CLEAR_SCREEN system("cls");
#endif

// FOR POWER PC COMPATIBLE COMPILERS
#if PPC == 1
#include <fcntl.h>
#define INIT_OS                                                                                                        \
    system("stty -echo -icanon");                                                                                      \
    fcntl(1, F_SETFL, O_NONBLOCK);
#define GET_CHAR getchar()
#define SET_STACK(s)                                                                                                   \
    __asm("addis r2,0,ha16(_temp)");                                                                                   \
    __asm("lwz r1,lo16(_temp)(r2)");
#define RESTORE_OS system("stty icanon echo"); // enable canonical mode and echo
#define LITTLE 0
#define CLEAR_SCREEN system("cls");
#endif

// FOR 64-BIT MAC COMPATIBLE COMPILERS (OSX Lion 10.7)
#if MAC == 1
#include <fcntl.h>
#define INIT_OS                                                                                                        \
    system("stty -echo -icanon");                                                                                      \
    fcntl(1, F_SETFL, O_NONBLOCK);
#define GET_CHAR getchar()
#define SET_STACK(s) __asm("movq _temp(%rip),%rsp");
#define RESTORE_OS system("stty icanon echo"); // enable canonical mode and echo
#define LITTLE 1
#define CLEAR_SCREEN system("cls");
#endif

#define SWAP_BYTES(v) 1 ? v : ((((v) >> 8) & 0x00ff)) | ((v) << 8)
#define SWAP_WORDS(v) LITTLE ? v : ((SWAP_BYTES(v) << 16)) | (SWAP_BYTES((v) >> 16))

#endif // __os345config_h__
