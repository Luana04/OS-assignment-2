#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

/* 
 * curr_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred
 * The first two indices determine the entry lane: first index is Side, second index is Direction
 * curr_arrivals[s][d] returns an array of all arrivals for the entry lane on side s for direction d,
 *   ordered in the same order as they arrived
 */
static Arrival curr_arrivals[4][4][20];

/*
 * semaphores[][]
 *
 * A 2D array that defines a semaphore for each entry lane,
 *   which are used to signal the corresponding traffic light that a car has arrived
 * The two indices determine the entry lane: first index is Side, second index is Direction
 */
static sem_t semaphores[4][4];

/*
 * supply_arrivals()
 *
 * A function for supplying arrivals to the intersection
 * This should be executed by a separate thread
 */
static void* supply_arrivals()
{
  int t = 0;
  int num_curr_arrivals[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_arrivals)/sizeof(Arrival); i++)
  {
    // get the next arrival in the list
    Arrival arrival = input_arrivals[i];
    // wait until this arrival is supposed to arrive
    sleep(arrival.time - t);
    t = arrival.time;
    // store the new arrival in curr_arrivals
    curr_arrivals[arrival.side][arrival.direction][num_curr_arrivals[arrival.side][arrival.direction]] = arrival;
    num_curr_arrivals[arrival.side][arrival.direction] += 1;
    // increment the semaphore for the traffic light that the arrival is for
    sem_post(&semaphores[arrival.side][arrival.direction]);
  }

  return(0);
}


/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void* manage_light(void* arg)
{
  // TODO:
  // while not all arrivals have been handled, repeatedly:
  //  - wait for an arrival using the semaphore for this traffic light
  //  - lock the right mutex(es)
  //  - make the traffic light turn green
  //  - sleep for CROSS_TIME seconds
  //  - make the traffic light turn red
  //  - unlock the right mutex(es)

  int lane_id = *(int*)arg;
  int side = lane_id / 4;        // Side of the intersection (0 to 3)
  int direction = lane_id % 4;  // Direction of the lane (0 to 3)

  while (true)
  {
      // Wait for an arrival using the semaphore for this traffic light
      sem_wait(&semaphores[side][direction]);

      // Lock the appropriate mutex for the critical section
      pthread_mutex_lock(&mutexes[side]);

      // Turn the traffic light green (simulate car crossing)
      printf("Traffic light on side %d direction %d: GREEN\n", side, direction);
      sleep(CROSS_TIME); // Simulate the time it takes for a car to cross

      // Turn the traffic light red
      printf("Traffic light on side %d direction %d: RED\n", side, direction);

      // Unlock the mutex
      pthread_mutex_unlock(&mutexes[side]);
  }

  return (0);
}


int main(int argc, char * argv[])
{

  pthread_t traffic_lights[16]; // 16 threads for the traffic lights (4 sides * 4 directions)
  pthread_t supply_thread;     // Thread for supplying arrivals
  int lane_ids[16];            // IDs for each lane
  int ret;

  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  // Initialize mutexes
  for (int i = 0; i < 4; i++)
  {
      pthread_mutex_init(&mutexes[i], NULL);
  }

  // start the timer
  start_time();
  
  // TODO: create a thread per traffic light that executes manage_light

  // TODO: create a thread that executes supply_arrivals

  // TODO: wait for all threads to finish
  // Create a thread per traffic light that executes manage_light
  for (int i = 0; i < 16; i++)
  {
      lane_ids[i] = i;
      ret = pthread_create(&traffic_lights[i], NULL, manage_light, &lane_ids[i]);
      if (ret != 0)
      {
          perror("Error creating traffic light thread");
          exit(EXIT_FAILURE);
      }
  }

  // Create a thread that executes supply_arrivals
  ret = pthread_create(&supply_thread, NULL, supply_arrivals, NULL);
  if (ret != 0)
  {
      perror("Error creating supply arrivals thread");
      exit(EXIT_FAILURE);
  }

  // Wait for all threads to finish
  for (int i = 0; i < 16; i++)
  {
      pthread_join(traffic_lights[i], NULL);
  }
  pthread_join(supply_thread, NULL);

  // destroy semaphores
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      sem_destroy(&semaphores[i][j]);
    }
  }

  // Destroy mutexes
  for (int i = 0; i < 4; i++)
  {
      pthread_mutex_destroy(&mutexes[i]);
  }

  return 0;
}
