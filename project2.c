#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define MIN_NOF_SEATS 50
#define MAX_NOF_SEATS 100
#define MIN_WAIT_TIME 50
#define MAX_WAIT_TIME 200

// ========================================== CUSTOM MESSAGE PROTOCOL BEGINS ==========================================
void* messages[MAX_NOF_SEATS];
pthread_mutex_t messageLockForClient[MAX_NOF_SEATS]; // all unlocked initially
pthread_mutex_t messageLockForServer[MAX_NOF_SEATS]; // all locked initially

/*    SAMPLE MESSAGE FLOW BETWEEN SERVER I AND CLIENT I
 *    1- message[i] = NULL, client sends to server // request a seat list - no choice for seat id
 *    2A- message[i] = seat_list, server sends to client // return seat list
 *    1- message[i] = (int)seat_id, client sends to server // request to reserve a seat - else request seat list
 *    2A- message[i] = seat_list, server sends to client // failed to reserve a seat - return seat lst
 *    2B- message[i] = NULL, server sends to client // successfully reserved seat - return null to notify client
 *
 *    repeats 1-2A until 2B is achieved.
 */
// CLIENT COMMANDS FOR MESSAGING
void sendRequest(int clientID, void* message) {
  messages[clientID-1] = message;  // write the request(message) - server is waiting
  pthread_mutex_unlock(messageLockForServer+clientID-1);  // trigger server - from now on a new response may come
}
void* receiveResponse(int clientID) {
  pthread_mutex_lock(messageLockForClient+clientID-1);  // wait for the lock - unlocked when a new response comes
  void* message = messages[clientID-1]; // read the response(message) and return it
  return message;
}
// SERVER COMMANDS FOR MESSAGING
void* receiveRequest(int clientID) {
  pthread_mutex_lock(messageLockForServer+clientID-1); // wait for the lock - unlocked when a new request comes
  void* message = messages[clientID-1];  // read the request(message) and return it
  return message;
}
void sendResponse(int clientID, void* message) {
  messages[clientID-1] = message;  // write the response(message) - client is waiting
  pthread_mutex_unlock(messageLockForClient+clientID-1); // trigger client - from now on a new request may come
}
/*  SAMPLE FLOW OF MUTEXES WHEN MESSAGING BETWEEN SERVER I AND CLIENT I
 *  initially serverLock = locked, clientLock = locked
 *
 *  1- client i sends a request
 *    * serverLock is unlocked - server can keep running by locking it via receiveRequest
 *  2- client i waits for response
 *    * clientLock is locked again - waiting for server to unlock it once
 *  3- server i waits for request
 *    * serverLock is locked again - waiting for client to unlock it once
 *    * serverLock is unlocked directly after acquiring lock
 *  4- server i sends a response
 *    * serverLock is
 */
// =========================================== CUSTOM MESSAGE PROTOCOL ENDS ===========================================

// ========================================= GLOBALS FOR SERVER & MAIN BEGINS =========================================
pthread_mutex_t seatLock[MAX_NOF_SEATS], reserveLock;

int seats[MAX_NOF_SEATS]; // reserved by which client

int nofReserved = 0;
int nofSeats;

typedef struct seat_list {
    char seats[MAX_NOF_SEATS],
    int nofSeats,
    int nofReferences,
            pthread_mutex_t referenceLock
} seat_list;

pthread_mutex_t changeLockForCurrentSeatList;
pthread_mutex_t updateLockForNextSeatList;

seat_list* currentSeatList;

void updateCurrentSeatList();
void initializeGlobals() {
  int i;
  for (i = 0; i < MAX_NOF_SEATS; ++i) {
    pthread_mutex_init(seatLock+i, NULL);
    seats[i] = 0;
    messages[i] = NULL;
    pthread_mutex_init(messageLockForClient+i, NULL);
    pthread_mutex_init(messageLockForServer+i, NULL);
    pthread_mutex_lock(messageLockForClient+i); // must be locked initially
    pthread_mutex_lock(messageLockForServer+i); // also, must be locked initially
  }
  pthread_mutex_init(&reserveLock, NULL);
  pthread_mutex_init(&changeLockForCurrentSeatList, NULL);
  pthread_mutex_init(&updateLockForNextSeatList, NULL);
  updateCurrentSeatList(); // initialize currentSeatList
}
// ========================================== GLOBALS FOR SERVER & MAIN ENDS ==========================================

// ================================== SEAT_LIST HANDLERS(READ-DISPOSE-UPDATE) BEGINS ==================================
seat_list* readSeatList() {
  pthread_mutex_lock(&changeLockForCurrentSeatList);
  seat_list* seatList = currentSeatList;
  pthread_mutex_lock(&(seatList->referenceLock));
  ++(seatList->nofReferences);
  pthread_mutex_unlock(&(seatList->referenceLock));
  pthread_mutex_unlock(&changeLockForReferencedSeatList);
  return seatList;
}
void disposeSeatList(seat_list* seatList) {
  pthread_mutex_lock(&(seatList->referenceLock));
  --(seatList->nofReferences);
  if (seatList->nofReferences == 0) {
    // it's safe to delete now - no other references to that list
    free(seatList);
  } else {
    pthread_mutex_unlock(&(seatList->referenceLock));
  }
}
void updateCurrentSeatList() {
  seat_list* nextSeatList;
  nextSeatList = (seat_list*)mallock(sizeof(seat_list));
  int i;
  nextSeatList->nofSeats = nofSeats;
  nextSeatList->nofReferences = 1;
  pthread_mutex_init(&(nextSeatList->referenceLock));
  pthread_mutex_lock(&updateLockForNextSeatList);
  for (i = 0; i < MAX_NOF_SEATS; ++i) {
    nextSeatList->seats[i] = seats[i] != 0;
    // != 0 => so, do not send the client info about which client reserved which seat
    // instead only send info about whether each seat is reserved (security & privacy)
  }
  pthread_mutex_lock(&changeLockForCurrentSeatList);
  seat_list* prevSeatList = currentSeatList;
  currentSeatList = nextSeatList;
  pthread_mutex_unlock(&changeLockForCurrentSeatList);
  pthread_mutex_unlock(&updateLockForNextSeatList);
  if (prevSeatList != NULL) {
    disposeSeatList(prevSeatList);
  }
}
// =================================== SEAT_LIST HANDLERS(READ-DISPOSE-UPDATE) ENDS ===================================

// ========================================= UTILITY/HELPER FUNCTIONS BEINGS ==========================================
int getRandom(int min, int max) {
  return (random() % (max - min + 1)) + min;
}

void millisleep(int milliseconds) {
#if _POSIX_C_SOURCE >= 199309L
  struct timespec sleepTime;
  sleepTime.tv_sec = milliseconds / 1000; // always 0 in our case, though.
  sleepTime.tv_nsec = (milliseconds % 1000) * 1000000; // milli to nano
  nanosleep(&sleepTime, NULL);
#else
  usleep(milliseconds * 1000);
#endif
}
// ========================================== UTILITY/HELPER FUNCTIONS ENDS ===========================================

// ==================================== SEAT CONTROL FUNCTIONS FOR SERVERS BEGINS =====================================
int checkSeat(int seatID) {
  return seats[seatID - 1] == 0;
}
int tryToReserveSeat(int seatID, int clientID) {
  if (seats[seatID - 1] != 0) {
    return 0; // failed to reserve because it's already reserved
  }

  pthread_mutex_lock(&reserveLock); // lock while an ongoing reserve to make sure prints are in correct order
  /* critical section for printing a reservation */
  fprintf(stdout, "Client%d reserves Seat%d\n", clientID, seatID);
  ++ nofReserved;
  /* critical section for printing a reservation ends */
  pthread_mutex_unlock(&reserveLock); // unlock reserve lock after print is finished (also count of reserved)

  seats[seatID - 1] = clientID;  // set the seat as reserved

  return 1; // successfully reserved
}
// ===================================== SEAT CONTROL FUNCTIONS FOR SERVERS ENDS ======================================

// =================================== THREAD DEFINITION FUNCTIONS AND MAIN BEGINS ==================================== 
void *server(void *param) {
  // the client for which this server thread is created
  int clientID = *((int*)param);

  int *seatID, reserved = 0;
  seat_list* list = NULL;

  while (reserved == 0) {
    seatID = receiveRequest(clientID); // A NEW REQUEST IS RECEIVED

    if (list != NULL) {
      // dispose/invalidate previous list as it will not be used anymore by the client or this server
      disposeSeatList(list);
      list = NULL;
    }
    if (seatID != NULL) {
      pthread_mutex_lock(&seatLock[*seatID - 1]);
      /* critical section for checking reservation status of seat seatID */
      reserved = tryToReserveSeat(*seatID, clientID);
      /* critical section for checking reservation status of seat seatID ends */
      pthread_mutex_unlock(&seatLock[*seatID - 1]);

      if (reserved) {
        updateCurrentSeatList(); // update currentSeatList (used for reads) with the new seat info
      }
    }
    if (reserved == 0) {
      list = readSeatList();
      // sends the message "YOU DID NOT/FAILED TO RESERVE A SEAT, SO, HERE'S THE LIST, PICK A SEAT" to the client.
    } // otherwise list is NULL,
      // which sends the message "YOU ALREADY RESERVED YOUR SEAT, SO, NO SEAT LIST FOR YOU" to the client.

    sendResponse(clientID, list); // A RESPONSE TO THE REQUEST IS SENT
  }
  // happy ending
  pthread_exit(0);
}

void *client(void *param) {
  // DO NOT ALLOW CLIENT TO ACCESS ANY GLOBAL DATA such as seats, or nofSeats (instead use the nofSeats sent via server)
  // client arrives
  int clientID = *((int*)param);

  // chose a random number of milliseconds to wait
  int waitTime = getRandom(MIN_WAIT_TIME, MAX_WAIT_TIME);

  // wait for that amount
  millisleep(waitTime);

  // start trying reservations
  int seatID, nofEmptySeats, i, seatIndex;

  sendRequest(clientID, NULL); // initial request
  seat_list* list = (seat_list*)receiveResponse(clientID); // initial response

  while (seat_list != NULL) {
    // RECEIVED A RESPONSE WITH A LIST, MEANING WE DID NOT/FAILED TO RESERVE A SEAT, SO WE SHOULD PICK A SEAT
    nofEmptySeats = 0; // determine how many seats are available
    for (i = 0; i < seat_list->nofSeats; ++i) {
      if (seat_list->seats[i] == 0) {
        ++nofEmptySeats;
      }
    }
    seatIndex = getRandom(1, nofEmptySeats);
    for (i = 0; i < seat_list->nofSeats; ++i) {
      if (seat_list->seats[i] == 0) {
        --seatIndex;
        if (seatIndex == 0) {
          seatID = i + 1;
        }
      }
    }
    sendRequest(clientID, &seatID);
    list = (seat_list*) receiveResponse(clientID);
  }

  pthread_exit(0);
}

int main(int argc, char* argv[]) {
  // HANDLE INPUT ARGUMENT
  if (argc != 2) {
    fprintf(stderr, "Usage: ./a.out <nofSeats: integer in range [50, 100]>\n");
    return -1;
  }
  nofSeats = atoi(argv[1]);
  if (nofSeats < MIN_NOF_SEATS) {
    fprintf(stderr, "Argument %s must be an integer that's at least %d\n", argv[1], MIN_NOF_SEATS);
    return -1;
  }
  if (nofSeats > MAX_NOF_SEATS) {
    fprintf(stderr, "Argument %s must be an integer that's at most %d\n", argv[1], MAX_NOF_SEATS);
    return -1;
  }

  // HANDLE INITIALIZATIONS OF GLOBAL VARIABLES AND RANDOM SEED
  initializeGlobals();
  srandom((unsigned)time(NULL)); // set current timestamp as the initial random seed

  // HANDLE DEFINITIONS FOR THREADS
  pthread_t clients[nofSeats], servers[nofSeats];
  pthread_attr_t clientAttrs[nofSeats], serverAttrs[nofSeats];
  int clientIDs[nofSeats];
  int i;

  fprintf(stdout, "Number of total seats: %d\n", nofSeats);

  // CREATE THREADS
  for (i = 0; i < nofSeats; ++i) {
    pthread_attr_init(clientAttrs+i); // default attributes for each thread
    pthread_attr_init(serverAttrs+i); // default attributes for each thread
    clientIDs[i] = i + 1;
    pthread_create(clients+i, clientAttrs+i, client, clientIDs+i);
    pthread_create(servers+i, serverAttrs+i, server, clientIDs+i);
  }

  // WAIT THREADS TO FINISH
  for (i = 0; i < nofSeats; ++i) {
    pthread_join(clients[i], NULL);
  }
  
  // CHECK IF ALL SEATS ARE RESERVED SUCCESSFULLY
  if (nofSeats == nofReserved) {
    fprintf(stdout, "All seats are reserved.\n");
  } else {
    fprintf(stderr, "Error! %d seats are reserved out of %d seats.", nofReserved, nofSeats);
    return -1;
  }
  return 0;
}
// ==================================== THREAD DEFINITION FUNCTIONS AND MAIN ENDS =====================================