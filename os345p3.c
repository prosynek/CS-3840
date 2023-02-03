// os345p3.c - Jurassic Park 07/27/2020
// ***********************************************************************
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
#include <time.h>

#include "os345.h"
#include "os345park.h"

// ***********************************************************************
// project 3 variables

// Jurassic Park
extern JPARK myPark;
extern Semaphore *parkMutex;            // protect park access
extern Semaphore *fillSeat[NUM_CARS];   // (signal) seat ready to fill
extern Semaphore *seatFilled[NUM_CARS]; // (wait) passenger seated
extern Semaphore *rideOver[NUM_CARS];   // (signal) ride over

//------------semaphores----------------
Semaphore* tickets;
Semaphore *roomInPark;         
Semaphore *roomInMuseum;        
Semaphore *roomInShop;          
Semaphore *needPassenger;       
Semaphore *passengerReady;     
Semaphore *needDriver;         
Semaphore *wakeDriver;          
Semaphore *driverReady;         
Semaphore* visitorRideDone;     
Semaphore* driverRideDone;      
Semaphore *needTicket;         
Semaphore *ticketReady;
Semaphore *buyTicket;           

// mutexes
Semaphore *carMutex;                
Semaphore *ticketBoothMutex;        
Semaphore *driverMutex;             
//--------------------------------------

int nextCar;    // car needing driver


//-------DELTA CLOCK---------
// DeltaClock deltaClock;
// Semaphore *deltaClockMutex;

// ***********************************************************************
// project 3 functions and tasks
void CL3_project3(int, char **);
void CL3_dc(int, char **);

void visitorTask(int, char **);
void workerTask(int, char **);
void carTask(int, char **);

// delta clock functions
void waitTime(int max, Semaphore* sem);


// ***********************************************************************
// ***********************************************************************
// project3 command
int P3_main(int argc, char *argv[])
{
    char buf[32];
    char *newArgv[2];

    //-----------------create semaphores-----------------
    tickets = createSemaphore("tickets", COUNTING, MAX_TICKETS);
    roomInPark = createSemaphore("roomInPark", COUNTING, MAX_IN_PARK);
    roomInMuseum = createSemaphore("roomInMuseum", COUNTING, MAX_IN_MUSEUM);
    roomInShop = createSemaphore("roomInShop", COUNTING, MAX_IN_GIFTSHOP); 
    needPassenger = createSemaphore("needPassenger", BINARY, 0);
    passengerReady = createSemaphore("passengerReady", BINARY, 0);
    needDriver = createSemaphore("needDriver", BINARY, 0);
    wakeDriver = createSemaphore("wakeDriver", BINARY, 0);         
    driverReady = createSemaphore("driverReady", BINARY, 0);
    needTicket = createSemaphore("needTicket", BINARY, 0);
    ticketReady = createSemaphore("ticketReady", BINARY, 0);
    buyTicket = createSemaphore("buyTicket", BINARY, 0);          
    ticketBoothMutex = createSemaphore("ticketBoothMutex", BINARY, 1);
    driverMutex= createSemaphore("driverMutex", BINARY, 1);
      

    // start park
    sprintf(buf, "jurassicPark");
    newArgv[0] = buf;
    createTask(buf,          // task name
               jurassicTask, // task
               MED_PRIORITY, // task priority
               1,            // task count
               newArgv);     // task argument

    // wait for park to get initialized...
    while (!parkMutex)
        SWAP;
    printf("\nStart Jurassic Park...");

    //?? create car, driver, and visitor tasks here

    // create visitor tasks
    for (int i = 0; i < NUM_VISITORS; i++)
    {
        sprintf(buf, "visitor%d", i);
        newArgv[0] = buf;
        createTask(buf,          // task name
                   visitorTask,  // task
                   MED_PRIORITY, // task priority
                   1,            // task count
                   newArgv);     // task argument
    }

    // create driver tasks
    for (int i = 0; i < NUM_DRIVERS; i++)
    {
        char name[32];
        char id[32];
        sprintf(name, "worker%d", i);
        sprintf(id, "%d", i);
        newArgv[0] = name;
        newArgv[1] = id;
        createTask(buf,          // task name
                   workerTask,   // task
                   MED_PRIORITY, // task priority
                   2,            // task count
                   newArgv);     // task argument
    }

    // create car tasks
    for (int i = 0; i < NUM_CARS; i++)
    {
        char name[32];
        char id[32];
        sprintf(name, "car%d", i);
        sprintf(id, "%d", i);
        newArgv[0] = name;
        newArgv[1] = id;
        createTask(buf,          // task name
                   carTask,      // task
                   MED_PRIORITY, // task priority
                   2,            // task count
                   newArgv);     // task argument
    }

    return 0;
} // end project3

void rwait()
{
    int t = (rand() % 5) + 1;
    time_t start_time, cur_time;
    time(&start_time);
    time(&cur_time);
    while (cur_time - start_time < t)
    {
        SWAP;
        time(&cur_time);
    }
}


//_____________________________________________carTask____________________________________________________
//________________________________________________________________________________________________________
void carTask(int argc, char *argv[])
{
    // make a mutex so only one worker can be selling tickets at a time
    int carid = atoi(argv[1]);

    Semaphore *visitorSems[NUM_SEATS];  // rideDone[]    
    Semaphore *driverSem;               // driverDone
    
    do
    {
        // wait till all seats are filled
        for (int i = 0; i < NUM_SEATS; i++)
        {
            // wait till seat is available - passenger ready
            SWAP; SEM_WAIT(fillSeat[carid]);

            // signal car needs a visitor (passenger)
            SWAP; SEM_SIGNAL(needPassenger);

            // wait for passenger to be ready
            SWAP; SEM_SIGNAL(passengerReady);
    
            // store global semaphores
            visitorSems[i] = visitorRideDone;        

            if (i == 2)
            {
                // car full
                SWAP; SEM_WAIT(driverMutex); 
                {
                    // signal driver needed
                    SWAP; SEM_SIGNAL(needDriver);

                    nextCar = carid + 1;

                    // wake up driver
                    SWAP; SEM_SIGNAL(wakeDriver);

                    // wait till driver is ready
                    SWAP; SEM_WAIT(driverReady);

                    // store gloabl semaphore
                    SWAP; driverSem = driverRideDone;

                }
                SWAP; SEM_SIGNAL(driverMutex);
            }

            // signal park the seat was filled
            SWAP; SEM_SIGNAL(seatFilled[carid]);   
        }

        // wait till ride is over
        SWAP; SEM_WAIT(rideOver[carid]);

        // signal the driver - ride over
        SWAP; SEM_SIGNAL(driverSem);

        // signal all the visitors ride is over
        for (int i=0; i < NUM_SEATS; i++) {
            SWAP; SEM_SIGNAL(visitorSems[i]);   
        }
        
    } while (1);
} // END [ carTask ]


//__________________________________________workerTask____________________________________________________
//________________________________________________________________________________________________________
void workerTask(int argc, char *argv[])
{
    // create semaphore for this worker task
    Semaphore *driverSem = createSemaphore(argv[0], 0, 0);

    // make a mutex so only one worker can be selling tickets at a time
    int driverid = atoi(argv[1]);

    do
    {
        

        if (semTryLock(needTicket)) {
            // someone needs ticket

            // send worker to sell tickets
            SWAP; SEM_WAIT(parkMutex);
            {
                SWAP; myPark.drivers[driverid] = -1;        
            }
            SWAP; SEM_SIGNAL(parkMutex);

            // signal ticket is ready to be purchased
            SWAP; SEM_SIGNAL(ticketReady);

            // wait till visitor buys ticket
            SWAP; SEM_WAIT(buyTicket);
        }
        else if (semTryLock(needDriver)) {
            // else if need a driver

            // assign driver to car
            SWAP; SEM_WAIT(parkMutex);
            {
                myPark.drivers[driverid] = nextCar;         // should this be + 1 ?
            }
            SWAP; SEM_SIGNAL(parkMutex);

            // set global semaphore
            driverRideDone = driverSem;

            // signal ready
            SWAP; SEM_SIGNAL(driverReady);

            // wait for driver to be done
            SWAP; SEM_WAIT(driverSem);
        } 

        // send driver to sleep
        SWAP; SEM_WAIT(parkMutex); 
        {
            SWAP; myPark.drivers[driverid] = 0;
        }
        SWAP; SEM_SIGNAL(parkMutex);
    } while (1); 
} // END [ workerTask ]


//__________________________________________visitorTask___________________________________________________
//________________________________________________________________________________________________________
void visitorTask(int argc, char *argv[])
{
    Semaphore *visitorSem = createSemaphore(argv[0], 0, 0);

    rwait();                                               // should be replaces by delta clock signaling
    
    // arrive @ park
    SWAP; SEM_WAIT(parkMutex);
    {
        SWAP; myPark.numOutsidePark++;
    }
    SWAP; SEM_SIGNAL(parkMutex);

    // rwait();
    SWAP; waitTime(100, visitorSem);

    // wait till there is room in the park
    SWAP; SEM_WAIT(roomInPark);
    {
        // visitor enters the park
        SWAP; SEM_WAIT(parkMutex);
        {
            SWAP; myPark.numOutsidePark--;      // exit line outside park 
            SWAP; myPark.numInPark++;           // enter park
            SWAP; myPark.numInTicketLine++;     // get in ticket line
        }
        SWAP; SEM_SIGNAL(parkMutex);

        // rwait();    // wait for random amount of time in ticket line
        SWAP; waitTime(30, visitorSem);

        // wait till tickets are available to be purchased
        SWAP; SEM_WAIT(tickets);
        {
            SWAP; SEM_WAIT(ticketBoothMutex);
            {   
                SWAP; SEM_SIGNAL(needTicket);   // signal need a ticket
                SWAP; SEM_WAIT(ticketReady);    // wait till ticket ready to beready
                SWAP; SEM_SIGNAL(buyTicket);    // buy ticket

                // ticket bought, exit ticket line & get in museum line
                SWAP; SEM_WAIT(parkMutex);
                {
                    SWAP; myPark.numTicketsAvailable--; // ticket bought
                    SWAP; myPark.numInTicketLine--;     // visitor leaves ticket line
                    SWAP; myPark.numInMuseumLine++;     // visitor gets in museum line
                }
                SWAP; SEM_SIGNAL(parkMutex);
            }
            SWAP; SEM_SIGNAL(ticketBoothMutex);
            
            // rwait();    // spend random time in museum line
            SWAP; waitTime(30, visitorSem);


            // wait until room in museum
            SWAP; SEM_WAIT(roomInMuseum);       

            // go to the museum
            SWAP; SEM_WAIT(parkMutex);          
            {
                SWAP; myPark.numInMuseumLine--;     // visitor leave museum line
                SWAP; myPark.numInMuseum++;         // visitor enters museum       
            }
            SWAP; SEM_SIGNAL(parkMutex);

            // rwait();    // wait random amount of time in museum
            SWAP; waitTime(30, visitorSem);

            // exit the museum & get in the car line
            SWAP; SEM_WAIT(parkMutex); 
            {
                SWAP; myPark.numInMuseum--;         // visitor exits museum
                SWAP; myPark.numInCarLine++;        // visitor gets in car line
            }
            SWAP; SEM_SIGNAL(parkMutex);

            // signal room in the museum
            SWAP; SEM_SIGNAL(roomInMuseum);     

            // wait for a car to signal it needs a passenger
            SWAP; SEM_WAIT(needPassenger); 

            // store the semaphore of the visitor globally
            SWAP; visitorRideDone = visitorSem;

            // signal car - ready
            SWAP; SEM_SIGNAL(passengerReady);
          
            // get in the car and give back the ticket
            SWAP; SEM_WAIT(parkMutex);
            {
                SWAP; myPark.numInCarLine--;            // exit car line
                SWAP; myPark.numInCars++;               // enter car
                SWAP; myPark.numTicketsAvailable++;     // free up ticket (ticket available)
            }
            SWAP; SEM_SIGNAL(parkMutex);
        }

        // signal tickets available
        SWAP; SEM_SIGNAL(tickets);

        // ?? do you have to wait for the ride to be over ?? 
        SWAP; SEM_WAIT(visitorRideDone);     // <-- need this ? check this

        // wait for car ride to be over & get in gift shop line
        SWAP; SEM_WAIT(parkMutex);
        {
            SWAP; myPark.numInCars--;       // visitor exits car
            SWAP; myPark.numInGiftLine++;   // visitor enters gift shop line
        }
        SWAP; SEM_SIGNAL(parkMutex);

        // rwait();    // wait for random amount of time in gift shop line
        SWAP; waitTime(30, visitorSem);

        // wait till there is room in the gift shop
        SWAP; SEM_WAIT(roomInShop);                     

        // go to gift shop
        SWAP; SEM_WAIT(parkMutex);
        {
            SWAP; myPark.numInGiftLine--;   // visitor exits gift shop line
            SWAP; myPark.numInGiftShop++;   // visitor enters gift shop
        }
        SWAP; SEM_SIGNAL(parkMutex);

        // rwait();    // wait for random amount of time in gift shop
        SWAP; waitTime(30, visitorSem);

        // exit gift shop & park
        SWAP; SEM_WAIT(parkMutex);
        {
            SWAP; myPark.numInGiftShop--;   // visitor exits gift shop
            SWAP; myPark.numInPark--;       // visitor exits park
            SWAP; myPark.numExitedPark++;   // increment number of visitors who have left the park
        }
        SWAP; SEM_SIGNAL(parkMutex);

        // signal there is room in the gift shop (visitor left)
        SWAP; SEM_SIGNAL(roomInShop);                               // !!
    }
    SWAP; SEM_SIGNAL(roomInPark);   // signal there is room in the park (visitor left park)
} // END [ visitorTask ]

// delta clock
void waitTime(int max, Semaphore* sem)
{
    // Randomize time before signaling a semaphore
    SWAP; int time = (rand() % max) + 1;
    SWAP; insertDC(time, sem);

    SWAP; SEM_WAIT(sem);
}

// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char *argv[])
{
    printf("\nDelta Clock");
    // ?? Implement a routine to display the current delta clock contents
    printf("\nTo Be Implemented!");
    return 0;
} // end CL3_dc

/*
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// ***********************************************************************
// delta clock command
int P3_dc(int argc, char* argv[])
{
    printf("\nDelta Clock");
    // ?? Implement a routine to display the current delta clock contents
    //printf("\nTo Be Implemented!");
    int i;
    for (i=0; i<numDeltaClock; i++)
    {
        printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
    }
    return 0;
} // end CL3_dc


// ***********************************************************************
// display all pending events in the delta clock list
void printDeltaClock(void)
{
    int i;
    for (i=0; i<numDeltaClock; i++)
    {
        printf("\n%4d%4d  %-20s", i, deltaClock[i].time, deltaClock[i].sem->name);
    }
    return;
}


// ***********************************************************************
// test delta clock
int P3_tdc(int argc, char* argv[])
{
    createTask( "DC Test",			// task name
        dcMonitorTask,		// task
        10,					// task priority
        argc,					// task arguments
        argv);

    timeTaskID = createTask( "Time",		// task name
        timeTask,	// task
        10,			// task priority
        argc,			// task arguments
        argv);
    return 0;
} // end P3_tdc



// ***********************************************************************
// monitor the delta clock task
int dcMonitorTask(int argc, char* argv[])
{
    int i, flg;
    char buf[32];
    // create some test times for event[0-9]
    int ttime[10] = {
        90, 300, 50, 170, 340, 300, 50, 300, 40, 110	};

    for (i=0; i<10; i++)
    {
        sprintf(buf, "event[%d]", i);
        event[i] = createSemaphore(buf, BINARY, 0);
        insertDeltaClock(ttime[i], event[i]);
    }
    printDeltaClock();

    while (numDeltaClock > 0)
    {
        SEM_WAIT(dcChange)
        flg = 0;
        for (i=0; i<10; i++)
        {
            if (event[i]->state ==1)			{
                    printf("\n  event[%d] signaled", i);
                    event[i]->state = 0;
                    flg = 1;
                }
        }
        if (flg) printDeltaClock();
    }
    printf("\nNo more events in Delta Clock");

    // kill dcMonitorTask
    tcb[timeTaskID].state = S_EXIT;
    return 0;
} // end dcMonitorTask


extern Semaphore* tics1sec;

// ********************************************************************************************
// display time every tics1sec
int timeTask(int argc, char* argv[])
{
    char svtime[64];						// ascii current time
    while (1)
    {
        SEM_WAIT(tics1sec)
        printf("\nTime = %s", myTime(svtime));
    }
    return 0;
} // end timeTask
*/
