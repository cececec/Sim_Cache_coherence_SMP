Sim_Cache_coherence_SMP
=======================

Simulation of Symmetric Cache coherence protocol

In Linux, use Make to create an executable called “smp_cache”.

In order to run your simulator, you need to execute the following command:
./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file>

Where:
smp_cache: Executable of the SMP simulator generated after making.
cache_size: Size of each cache in the system (all caches are of the same size)
assoc: Associativity of each cache (all caches are of the same associativity)
block_size: Block size of each cache line (all caches are of the same block size)
num_processors: Number of processors in the system (represents how many
caches should be instantiated)
protocol: Coherence protocol to be used (0: MSI, 1:MESI, 2:MOESI)
trace_file: The input file that has the multi threaded workload trace.
