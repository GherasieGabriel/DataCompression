
#include "mpi.h"
#include <stdio.h>

#define SIZE 12  // Use #define for array sizes (no malloc needed!)

int main(int argc, char* argv[]) {
    int rank, numproc;
    
    // Static arrays - no malloc/free needed!
    int data[SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    int result[SIZE] = {0};
    
    // Step 1: Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Get my ID
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);  // How many total?

    // Step 2: Process 0 sets up data (already done above)
    if (rank == 0) {
        printf("Starting computation...\n");
    }

    // Step 3: Divide work
    int chunk = SIZE / numproc;
    int local_data[10];  // Buffer for my chunk
    
    // Step 4: Distribute work
    MPI_Scatter(data, chunk, MPI_INT, local_data, chunk, MPI_INT, 0, MPI_COMM_WORLD);

    // Step 5: Do the work
    // (Process your chunk here)

    // Step 6: Collect results
    MPI_Gather(local_data, chunk, MPI_INT, result, chunk, MPI_INT, 0, MPI_COMM_WORLD);

    // Step 7: Print results
    if (rank == 0) {
        printf("Done!\n");
    }

    // Step 8: Cleanup
    MPI_Finalize();
    return 0;
}

/*
╔══════════════════════════════════════════════════════════════════════════╗
║                         MPI FUNCTION GUIDE                                ║
╚══════════════════════════════════════════════════════════════════════════╝

1. INITIALIZATION & CLEANUP
────────────────────────────────────────────────────────────────────────────
   MPI_Init(&argc, &argv)           Start MPI
   MPI_Finalize()                   End MPI (always call at end!)
   MPI_Comm_rank(comm, &rank)       Get my process ID (0, 1, 2, 3...)
   MPI_Comm_size(comm, &numproc)    Get total number of processes


2. BROADCASTING (One-to-All)
────────────────────────────────────────────────────────────────────────────
   MPI_Bcast(&data, count, type, root, comm)
   
   → Sends SAME data from root to ALL processes
   → Everyone gets a COPY of the data
   
   Example:
   int x = 5;  // Only on process 0
   MPI_Bcast(&x, 1, MPI_INT, 0, MPI_COMM_WORLD);
   // Now ALL processes have x = 5


3. SCATTERING (One-to-Many, Different Data)
────────────────────────────────────────────────────────────────────────────
   MPI_Scatter(send_data, send_count, send_type,
               recv_data, recv_count, recv_type,
               root, comm)
   
   → Divides array and sends DIFFERENT chunks to each process
   → Process 0 gets chunk 0, Process 1 gets chunk 1, etc.
   
   Example:
   int data[12] = {1,2,3,4,5,6,7,8,9,10,11,12};  // On process 0
   int local[3];
   MPI_Scatter(data, 3, MPI_INT, local, 3, MPI_INT, 0, MPI_COMM_WORLD);
   // Process 0 gets: [1,2,3]
   // Process 1 gets: [4,5,6]
   // Process 2 gets: [7,8,9]
   // Process 3 gets: [10,11,12]


4. GATHERING (Many-to-One, Equal Sizes)
────────────────────────────────────────────────────────────────────────────
   MPI_Gather(send_data, send_count, send_type,
              recv_data, recv_count, recv_type,
              root, comm)
   
   → Collects SAME-SIZE chunks from all processes
   → Opposite of Scatter
   
   Example:
   int local[3] = {1, 2, 3};  // Each process has 3 numbers
   int result[12];  // Only on process 0
   MPI_Gather(local, 3, MPI_INT, result, 3, MPI_INT, 0, MPI_COMM_WORLD);
   // Process 0's result: [1,2,3,4,5,6,7,8,9,10,11,12]


5. REDUCTION (Combine with Operation)
────────────────────────────────────────────────────────────────────────────
   MPI_Reduce(&send_data, &recv_data, count, type, operation, root, comm)
   
   → Combines values from all processes using an operation
   → Operations: MPI_MAX, MPI_MIN, MPI_SUM, MPI_PROD
   
   Example:
   int local_max = 7;  // Different on each process
   int global_max;
   MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
   // If processes have: 3, 7, 5, 2
   // Result on process 0: global_max = 7


╔══════════════════════════════════════════════════════════════════════════╗
║                           DATA TYPES                                      ║
╚══════════════════════════════════════════════════════════════════════════╝

   MPI_INT              int
   MPI_FLOAT            float
   MPI_DOUBLE           double
   MPI_CHAR             char
   MPI_UNSIGNED_CHAR    unsigned char


╔══════════════════════════════════════════════════════════════════════════╗
║                        COMMON PATTERNS                                    ║
╚══════════════════════════════════════════════════════════════════════════╝

PATTERN 1: Find Maximum Value in Array
────────────────────────────────────────────────────────────────────────────
int data[12] = {3,5,2,8,5,1,7,5,9,4,5,6};
int chunk = 12 / numproc;
int start = rank * chunk;
int end = (rank == numproc-1) ? 12 : start + chunk;

int local_max = data[start];
for (int i = start+1; i < end; i++) {
    if (data[i] > local_max) local_max = data[i];
}

int global_max;
MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);


PATTERN 2: Array Addition
────────────────────────────────────────────────────────────────────────────
int a[12] = {1,2,3,4,5,6,7,8,9,10,11,12};
int b[12] = {10,20,30,40,50,60,70,80,90,100,110,120};
int result[12];

int chunk = 12 / numproc;
int local_a[10], local_b[10], local_result[10];

MPI_Scatter(a, chunk, MPI_INT, local_a, chunk, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Scatter(b, chunk, MPI_INT, local_b, chunk, MPI_INT, 0, MPI_COMM_WORLD);

for (int i = 0; i < chunk; i++) {
    local_result[i] = local_a[i] + local_b[i];
}

MPI_Gather(local_result, chunk, MPI_INT, result, chunk, MPI_INT, 0, MPI_COMM_WORLD);


PATTERN 3: Count Elements
────────────────────────────────────────────────────────────────────────────
int data[12] = {1,2,3,2,5,2,7,2,9,2,11,2};
int chunk = 12 / numproc;
int local_data[10];

MPI_Scatter(data, chunk, MPI_INT, local_data, chunk, MPI_INT, 0, MPI_COMM_WORLD);

int local_count = 0;
for (int i = 0; i < chunk; i++) {
    if (local_data[i] == 2) local_count++;
}

int total_count;
MPI_Reduce(&local_count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


╔══════════════════════════════════════════════════════════════════════════╗
║                         TIPS & TRICKS                                     ║
╚══════════════════════════════════════════════════════════════════════════╝

1. Use #define for array sizes (no need for malloc!)
2. Use "if (rank == 0)" for printing or initialization
3. Always call MPI_Init() first and MPI_Finalize() last
4. Array size should divide evenly by number of processes for simplicity
5. Use static arrays instead of malloc when possible
6. Test with small arrays first (like 8 or 12 elements)

╔══════════════════════════════════════════════════════════════════════════╗
║                      RUNNING YOUR PROGRAM                                 ║
╚══════════════════════════════════════════════════════════════════════════╝

Command line:
   mpiexec -n 4 program.exe
           │  │  └─ Your program name
           │  └─ Number of processes (2, 4, 8, etc.)
           └─ MPI execution command

Example:
   mpiexec -n 4 MPI-2.exe     (Run with 4 processes)
   mpiexec -n 2 MPI.exe       (Run with 2 processes)

*/
