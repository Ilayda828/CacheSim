# CacheSim
A program that simulates the hit, miss, and eviction behavior of L1 and L2 caches using memory access traces.

📌 Project Purpose
In this project, you are asked to write a program that simulates the hit, miss and eviction behaviors of L1 and L2 caches using a given RAM image and memory access traces (memory trace) file.

In other words, your simulator will work like a cache in a real processor; it will track the memory accesses made, check if they are in the caches, and determine which data to discard (eviction) if the cache is full.

📌 Inputs and Outputs of the Simulator
Inputs:

RAM.dat file (binary file) containing the initial image of the RAM.

Trace file containing the memory accesses to be processed (e.g. test1.trace).

Properties of L1 and L2 caches (such as number of sets, block size, associativity) with command line parameters.

Outputs:

Total hit, miss and eviction numbers for L1 instruction cache (L1I), L1 data cache (L1D) and L2 cache.

The contents of each cache at the end of the simulation (tag, valid bit, data, etc.).

Detailed output about cache behaviors step by step (hit/miss status, eviction information, which cache the data is placed in, etc.).
📌 Command Line Arguments
Your simulator will be run with the following parameters:

-L1s <L1s>: Number of set index bits of the L1 cache (number of sets = 2^s)

-L1E <L1E>: Level of associativeness of the L1 cache (number of lines in each set)

-L1b <L1b>: Number of L1 block size bits (block size = 2^b)

-L2s <L2s>, -L2E <L2E>, -L2b <L2b>: For the same L2.

-t <tracefile>: Trace file to be processed.

📌 Cache Features and Rules
L1 cache is modeled separately as data and instruction cache.

L2 cache is unified (common for both data and code).

Write-through and no write allocate policies will be applied in caches.

Eviction policy will be FIFO (First In First Out).

Memory accesses are aligned so that they do not cross block boundaries.

📌 Structure of Trace File
Each line contains one or two memory accesses:

I : Instruction load

L : Data load

S : Data store

M : Data modify (load + store)

Each line contains data for address, size and store.

Example:
L 000ebe20, 3, 58a35a
I 00002010, 6
S 0003b020, 7, abb2cdc69bb454

📌 Project Working Steps
Load memory image (RAM.dat) and trace file.

Create L1 and L2 caches according to the parameters given with the command line.

Process each memory access in the trace file in order:

First check in L1I or L1D cache.

If there is a miss, check in L2 cache.

If there is no L2, fetch from RAM.

If necessary, perform eviction (with FIFO policy).

Update hit/miss/eviction counter after each access.

At the end of the process, output the status of all caches.

📌 Evaluation Criteria
The program must work correctly for different s, E, b values.

Memory management (malloc/free) must be done correctly.

The program must compile without warnings and errors.

Hit/miss/eviction numbers must be calculated correctly.

Simulation outputs must be in the specified format.

Code naming and delivery rules must be followed.
