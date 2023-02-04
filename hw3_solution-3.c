#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <float.h>
#include <string.h>

// Definition of a Queue Node including arrival and service time
struct QueueNode
{
  double arrival_time; // customer arrival time, measured from time t=0, inter-arrival times exponential
  double service_time; // customer service time (exponential)
  double time_aft_prev;
  double start_ser;
  double wait_time;
  double resp_time;
  double arrexp;
  struct QueueNode *next; // next element in line; NULL if this is the last element
};

// Suggested queue definition
// Feel free to use some of the functions you implemented in HW2 to manipulate the queue
// You can change the queue definition and add/remove member variables to suit your implementation
struct Queue
{

  struct QueueNode *head; // Point to the first node of the element queue
  struct QueueNode *tail; // Point to the tail node of the element queue

  struct QueueNode *first; // Point to the first arrived customer that is waiting for service
  struct QueueNode *last;  // Point to the last arrrived customer that is waiting for service
  int waiting_count;       // Current number of customers waiting for service

  double cumulative_response;   // Accumulated response time for all effective departures
  double cumulative_waiting;    // Accumulated waiting time for all effective departures
  double cumulative_idle_times; // Accumulated times when the system is idle, i.e., no customers in the system
  double cumulative_number;     // Accumulated number of customers in the system
};

// ------------Global variables------------------------------------------------------
// Feel free to add or remove
static double computed_stats[4];  // Store computed statistics: [E(n), E(r), E(w), p0]
static double simulated_stats[4]; // Store simulated statistics [n, r, w, sim_p0]
int departure_count = 0;          // current number of departures from queue
int arrival_count = 0;
double current_time = 0; // current time during simulation
int total_exp = 0;
double cumresp = 0;
double cumwait = 0;
double cumidle = 0;
//-----------------Queue Functions------------------------------------
// Feel free to add more functions or redefine the following functions

void InsertAtTail(struct Queue *q, struct QueueNode *node)
{

  if (q->head == NULL)
  {
    q->head = node;
    q->tail = q->head;
    return;
  }
  else
  {
    q->tail->next = node;
    q->tail = node;
  }
  current_time++;
  // printf("%ld\n", current_time);
}

double exponential(double x)
{
  total_exp++;
  double u = (double)rand() / (double)(RAND_MAX + 1.0);
  return log(1 - u) / -x;
}

// The following function initializes all "D" (i.e., total_departure) elements in the queue
// 1. It uses the seed value to initialize random number generator
// 2. Generates "D" exponentially distributed inter-arrival times based on lambda
//    And inserts "D" elements in queue with the correct arrival times
//    Arrival time for element i is computed based on the arrival time of element i+1 added to element i's generated inter-arrival time
//    Arrival time for first element is just that element's generated inter-arrival time
// 3. Generates "D" exponentially distributed service times based on mu
//    And updates each queue element's service time in order based on generated service times
// 4. Returns a pointer to the generated queue
struct Queue *InitializeQueue(int seed, float lambda, float mu, int total_departures)
{
  if (total_departures == 0)
  {
    return NULL;
  }
  double prev_arrtime = 0;
  struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));

  queue->head = NULL;
  queue->tail = NULL;
  queue->cumulative_waiting = 0;
  queue->cumulative_idle_times = 0;
  queue->cumulative_response = 0;

  // Initialize a new Queue by inserting all the nodes with arrival times
  double exp = exponential(lambda);
  for (int i = 0; i < total_departures; i++)
  {
    struct QueueNode *node = (struct QueueNode *)malloc(sizeof(struct QueueNode));
    node->arrival_time = prev_arrtime + exp;
    node->arrexp = exp;
    prev_arrtime = node->arrival_time;
    if (i != total_departures - 1)
    {
      exp = exponential(lambda);
    }
    InsertAtTail(queue, node);
  }

  struct QueueNode *temp = queue->head;
  exp = exponential(mu);
  double last_ser_fin = 0;
  // Initialize the Queue with service times and calculate other simulation values
  while (temp != NULL)
  {
    if (temp->arrival_time > last_ser_fin)
    {
      temp->start_ser = temp->arrival_time;
    }
    else
    {
      temp->start_ser = last_ser_fin;
    }
    temp->wait_time = temp->start_ser - temp->arrival_time;

    temp->service_time = exp;
    temp->resp_time = temp->wait_time + temp->service_time;

    if (temp->wait_time <= 0)
    {
      temp->time_aft_prev = temp->arrival_time - last_ser_fin;
    }
    else
    {
      temp->time_aft_prev = 0;
    }
    last_ser_fin = temp->start_ser + temp->service_time;
    temp = temp->next;
    exp = exponential(mu);
  }
  return queue;
}

void printServiceArrival(struct Queue *q, int n)
{
  struct QueueNode *temp = q->head;
  double total_time = 0;
  while (temp)
  {
    printf("Arrival: %f   Service start: %f   Service finish: %f   Wait time: %f\n", temp->arrival_time, temp->start_ser,
           temp->start_ser + temp->service_time, temp->wait_time);
    //  printf("Arrival: %f   Interarrival: %f   RAND: %f   Next arrival: %f\n", temp->arrival_time, temp->time_bef_next, temp->u, temp->time_bef_next + temp->arrival_time);
    total_time += temp->wait_time;
    temp = temp->next;
  }
  // total_time = (q->tail->start_ser+q->tail->service_time) - q->head->arrival_time;

  printf("simulated wait time: %.4f\n", q->cumulative_waiting / 2000);
  printf("%d\n", total_exp);
}
// Use the M/M/1 formulas from class to compute E(n), E(r), E(w), p0
void GenerateComputedStatistics(float lambda, float mu)
{
  double rho = lambda / mu;
  computed_stats[0] = rho / (1 - rho);
  computed_stats[1] = 1 / ((1 - rho) * mu);
  computed_stats[2] = rho / ((1 - rho) * mu);
  computed_stats[3] = 1 - rho;
}

// This function should be called to print periodic and/or end-of-simulation statistics
// Do not modify output format
void PrintStatistics(struct Queue *elementQ, int total_departures, int print_period, float lambda)
{

  printf("\n");
  if (departure_count == total_departures)
    printf("End of Simulation - after %d departures\n", departure_count);
  else
    printf("After %d departures\n", departure_count);

  printf("Mean n = %.4f (Simulated) and %.4f (Computed)\n", simulated_stats[0], computed_stats[0]);
  printf("Mean r = %.4f (Simulated) and %.4f (Computed)\n", simulated_stats[1], computed_stats[1]);
  printf("Mean w = %.4f (Simulated) and %.4f (Computed)\n", simulated_stats[2], computed_stats[2]);
  printf("p0 = %.4f (Simulated) and %.4f (Computed)\n", simulated_stats[3], computed_stats[3]);
}

// This function is called from simulator if the next event is an arrival
// Should update simulated statistics based on new arrival
// Should update current queue nodes and various queue member variables
// *arrival points to queue node that arrived
// Returns pointer to node that will arrive next
struct QueueNode *ProcessArrival(struct Queue *elementQ, struct QueueNode *arrival)
{
  return arrival->next;
}

// // This function is called from simulator if next event is "start_service"
// //  Should update queue statistics
void StartService(struct Queue *elementQ)
{
}

// // This function is called from simulator if the next event is a departure
// // Should update simulated queue statistics
// // Should update current queue nodes and various queue member variables
void ProcessDeparture(struct Queue *elementQ, struct QueueNode *temp)
{
}

// // This is the main simulator function
// // Should run until departure_count == total_departures
// // Determines what the next event is based on current_time
// // Calls appropriate function based on next event: ProcessArrival(), StartService(), ProcessDeparture()
// // Advances current_time to next event
// // Updates queue statistics if needed
// // Print statistics if departure_count is a multiple of print_period
// // Print statistics at end of simulation (departure_count == total_departure)
void Simulation(struct Queue *elementQ, float lambda, float mu, int print_period, int total_departures)
{
  int n = 0;
  struct QueueNode *temp = elementQ->head;
  current_time = 0;
  double depart_time = elementQ->tail->service_time + elementQ->tail->start_ser;
  double t1 = 0;
  double pp = print_period;

  while (departure_count != total_departures)
  {
    if (t1 < depart_time)
    {
      current_time = t1;
      n++;
      arrival_count++;
      elementQ->cumulative_waiting += temp->wait_time;
      elementQ->cumulative_response += temp->resp_time;
      elementQ->cumulative_idle_times += temp->time_aft_prev;
      if (temp->next != NULL)
      {
        temp = temp->next;
      }

      t1 = current_time + temp->arrexp;
      if (n == 1)
      {
        depart_time = current_time + temp->service_time;
      }
    }
    else
    {
      departure_count++;

      current_time = depart_time;
      n--;
      if (n > 0)
      {
        depart_time = current_time + temp->service_time;
      }
      else
      {
        depart_time = elementQ->tail->service_time + elementQ->tail->start_ser;
      }

      if (floor(departure_count / (pp)) == departure_count / (pp) && departure_count != 0 && departure_count != total_departures && pp != 0)
      {
        simulated_stats[2] = elementQ->cumulative_waiting / (departure_count);
        simulated_stats[1] = elementQ->cumulative_response / (departure_count);
        simulated_stats[3] = elementQ->cumulative_idle_times / ((temp->start_ser + temp->service_time));
        simulated_stats[0] = (departure_count / (temp->start_ser + temp->service_time)) * simulated_stats[1];
        PrintStatistics(elementQ, total_departures, print_period, lambda);
      }
    }
  }
  simulated_stats[2] = elementQ->cumulative_waiting / (total_departures);
  simulated_stats[1] = elementQ->cumulative_response / (total_departures);
  simulated_stats[3] = elementQ->cumulative_idle_times / ((elementQ->tail->start_ser + elementQ->tail->service_time));
  simulated_stats[0] = (total_departures / (elementQ->tail->start_ser + elementQ->tail->service_time)) * simulated_stats[1];
  PrintStatistics(elementQ, total_departures, print_period, lambda);
}

// // Free memory allocated for queue at the end of simulation
void FreeQueue(struct Queue *elementQ)
{
  struct QueueNode *temp = elementQ->head;
  struct QueueNode *temp2 = elementQ->head;
  while (temp2)
  {
    temp = temp2->next;
    free(temp2);
    temp2 = temp;
  }
  free(elementQ);
}

// Program's main function
int main(int argc, char *argv[])
{
  // input arguments lambda, mu, D1, D, S
  if (argc >= 6)
  {

    float lambda = atof(argv[1]);
    float mu = atof(argv[2]);
    int print_period = atoi(argv[3]);
    int total_departures = atoi(argv[4]);
    int random_seed = atoi(argv[5]);
    srand(random_seed);

    // Add error checks for input variables here, exit if incorrect input
    if (mu <= 0 || lambda <= 0 || total_departures < 1 || print_period < 0)
    {
      printf("wrong input(s)!\n");
      return 0;
    }
    // If no input errors, generate M/M/1 computed statistics based on formulas from class
    GenerateComputedStatistics(lambda, mu);

    // // Start Simulation
    printf("Simulating M/M/1 queue with lambda = %f, mu = %f, D1 = %d, D = %d, S = %d\n", lambda, mu, print_period, total_departures, random_seed);
    struct Queue *elementQ = InitializeQueue(random_seed, lambda, mu, total_departures);
    Simulation(elementQ, lambda, mu, print_period, total_departures);
    // printServiceArrival(elementQ, total_departures);
    //  printf("total time: %f\n", current_time);
    FreeQueue(elementQ);
  }
  else
    printf("Insufficient number of arguments provided!\n");

  return 0;
}
