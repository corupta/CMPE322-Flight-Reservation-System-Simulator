#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define MIN_NOF_SEATS 50
#define MAX_NOF_SEATS 100
#define MIN_WAIT_TIME 50
#define MAX_WAIT_TIME 200

pthread_mutex_t seatLock[MAX_NOF_SEATS], reserveLock;

int seats[MAX_NOF_SEATS]; // reserved by which client

int nofReserved = 0;
int nofSeats;

void initializeGlobals() {
  int i;
  for (i = 0; i < MAX_NOF_SEATS; ++i) {
    pthread_mutex_init(&seatLock[i], NULL);
    seats[i] = 0;
  }
  pthread_mutex_init(&reserveLock, NULL);
}

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

void *client(void *param) {
  // client arrives
  int clientID = *((int*)param);

  // chose a random number of miliseconds to wait
  int waitTime = getRandom(MIN_WAIT_TIME, MAX_WAIT_TIME);

  // wait for that amount
  millisleep(waitTime);

  // start trying reservations
  int seatID, reserved = 0;

  while (reserved == 0) {
    seatID = getRandom(1, nofSeats);

    pthread_mutex_lock(&seatLock[seatID - 1]);

    /* critical section for checking reservation status of seat seatID */

    reserved = tryToReserveSeat(seatID, clientID);

    /* critical section for checking reservation status of seat seatID ends */

    pthread_mutex_unlock(&seatLock[seatID - 1]);
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
  pthread_t clients[nofSeats];
  pthread_attr_t attrs[nofSeats];
  int clientIDs[nofSeats];
  int i;

  fprintf(stdout, "Number of total seats: %d\n", nofSeats);

  // CREATE THREADS
  for (i = 0; i < nofSeats; ++i) {
    pthread_attr_init(&attrs[i]); // default attributes for each thread
    clientIDs[i] = i + 1;
    pthread_create(&clients[i], &attrs[i], client, &clientIDs[i]);
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
