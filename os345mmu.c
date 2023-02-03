// os345mmu.c - LC-3 Memory Management Unit	06/21/2020
//
//		03/12/2015	added PAGE_GET_SIZE to accessPage()
//
// **************************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the CS345 projects.          **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************
#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os345.h"
#include "os345lc3.h"

#pragma clang diagnostic ignored "-Wpointer-to-int-cast"

// ***********************************************************************
// mmu variables

// LC-3 memory
unsigned short int memory[LC3_MAX_MEMORY];

// statistics
int memAccess;     // memory accesses
int memHits;       // memory hits
int memPageFaults; // memory faults
int clockRPT;      // RPT clock
int clockUPT;      // UPT clock

int getFrame(int);
int getAvailableFrame(void);
extern TCB tcb[];   // task control block
extern int curTask; // current task #

int getFrame(int notme)
{
    int frame;
    frame = getAvailableFrame();
    if (frame >= 0)
        return frame;

    // run clock
    printf("\nWe're toast!!!!!!!!!!!!");

    return frame;
}
// **************************************************************************
// **************************************************************************
// LC3 Memory Management Unit
// Virtual Memory Process
// **************************************************************************
//           ___________________________________Frame defined
//          / __________________________________Dirty frame
//         / / _________________________________Referenced frame
//        / / / ________________________________Pinned in memory
//       / / / /     ___________________________
//      / / / /     /                 __________frame # (0-1023) (2^10)
//     / / / /     /                 / _________page defined
//    / / / /     /                 / /       __page # (0-4096) (2^12)
//   / / / /     /                 / /       /
//  / / / /     / 	             / /       /
// F D R P - - f f|f f f f f f f f|S - - - p p p p|p p p p p p p p

#define MMU_ENABLE 0

unsigned short int *getMemAdr(int va, int rwFlg)
{
#if !MMU_ENABLE
    return &memory[va];
#else
    unsigned short int pa;
    int rpta, rpte1, rpte2;
    int upta, upte1, upte2;
    int rptFrame, uptFrame;

    // turn off virtual addressing for system RAM
    if (va < 0x3000)
        return &memory[va];
    rpta = tcb[curTask].RPT + RPTI(va); // root page table address
    rpte1 = memory[rpta];               // FDRP__ffffffffff
    rpte2 = memory[rpta + 1];           // S___pppppppppppp
    if (DEFINED(rpte1))
    {
    } // rpte defined
    else
    {
    }                              // rpte undefined
    memory[rpta] = SET_REF(rpte1); // set rpt frame access bit

    upta = (FRAME(rpte1) << 6) + UPTI(va); // user page table address
    upte1 = memory[upta];                  // FDRP__ffffffffff
    upte2 = memory[upta + 1];              // S___pppppppppppp
    if (DEFINED(upte1))
    {
    } // upte defined
    else
    {
    }                              // upte undefined
    memory[upta] = SET_REF(upte1); // set upt frame access bit
    return &memory[(FRAME(upte1) << 6) + FRAMEOFFSET(va)];
#endif
} // end getMemAdr

// **************************************************************************
// **************************************************************************
// set frames available from sf to ef
//    flg = 0 -> clear all others
//        = 1 -> just add bits
//
void setFrameTableBits(int flg, int sf, int ef)
{
    int i, data;
    int adr = LC3_FBT - 1; // index to frame bit table
    int fmask = 0x0001;    // bit mask

    // 1024 frames in LC-3 memory
    for (i = 0; i < LC3_FRAMES; i++)
    {
        if (fmask & 0x0001)
        {
            fmask = 0x8000;
            adr++;
            data = (flg) ? MEMWORD(adr) : 0;
        }
        else
            fmask = fmask >> 1;
        // allocate frame if in range
        if ((i >= sf) && (i < ef))
            data = data | fmask;
        MEMWORD(adr) = data;
    }
    return;
} // end setFrameTableBits

// **************************************************************************
// get frame from frame bit table (else return -1)
int getAvailableFrame()
{
    int i, data;
    int adr = LC3_FBT - 1; // index to frame bit table
    int fmask = 0x0001;    // bit mask

    for (i = 0; i < LC3_FRAMES; i++) // look thru all frames
    {
        if (fmask & 0x0001)
        {
            fmask = 0x8000; // move to next work
            adr++;
            data = MEMWORD(adr);
        }
        else
            fmask = fmask >> 1; // next frame
        // deallocate frame and return frame #
        if (data & fmask)
        {
            MEMWORD(adr) = data & ~fmask;
            return i;
        }
    }
    return -1;
} // end getAvailableFrame

// **************************************************************************
// read/write to swap space
int accessPage(int pnum, int frame, int rwnFlg)
{
    static int nextPage;   // swap page size
    static int pageReads;  // page reads
    static int pageWrites; // page writes
    static unsigned short int swapMemory[LC3_MAX_SWAP_MEMORY];

    if ((nextPage >= LC3_MAX_PAGE) || (pnum >= LC3_MAX_PAGE))
    {
        printf("\nVirtual Memory Space Exceeded!  (%d)", LC3_MAX_PAGE);
        exit(-4);
    }
    switch (rwnFlg)
    {
    case PAGE_INIT:        // init paging
        clockRPT = 0;      // clear RPT clock
        clockUPT = 0;      // clear UPT clock
        memAccess = 0;     // memory accesses
        memHits = 0;       // memory hits
        memPageFaults = 0; // memory faults
        nextPage = 0;      // disk swap space size
        pageReads = 0;     // disk page reads
        pageWrites = 0;    // disk page writes
        return 0;

    case PAGE_GET_SIZE: // return swap size
        return nextPage;

    case PAGE_GET_READS: // return swap reads
        return pageReads;

    case PAGE_GET_WRITES: // return swap writes
        return pageWrites;

    case PAGE_GET_ADR: // return page address
        return (int)(&swapMemory[pnum << 6]);

    case PAGE_NEW_WRITE: // new write (Drops thru to write old)
        pnum = nextPage++;

    case PAGE_OLD_WRITE: // write
        // printf("\n    (%d) Write frame %d (memory[%04x]) to page %d", p.PID, frame, frame<<6, pnum);
        memcpy(&swapMemory[pnum << 6], &memory[frame << 6], 1 << 7);
        pageWrites++;
        return pnum;

    case PAGE_READ: // read
        // printf("\n    (%d) Read page %d into frame %d (memory[%04x])", p.PID, pnum, frame, frame<<6);
        memcpy(&memory[frame << 6], &swapMemory[pnum << 6], 1 << 7);
        pageReads++;
        return pnum;

    case PAGE_FREE: // free page
        printf("\nPAGE_FREE not implemented");
        break;
    }
    return pnum;
} // end accessPage
