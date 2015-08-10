#!/bin/bash

#----------------------------------------------------
# symmetric.slurm
# Generic symmetric script – MPI + OpenMP
#----------------------------------------------------
#SBATCH -J ALLGRAPHS_3 # Job name
#SBATCH -o ALLGRAPHS_3.%j.out # stdout; %j expands to jobid
#SBATCH -e ALLGRAPHS_3.%j.err # stderr; skip to combine stdout and stderr
#SBATCH -p normal-mic # queue
#SBATCH -N 3 # Number of nodes, not cores (16 cores/node)
#SBATCH -n 3 # Total number of MPI tasks (if omitted, n=N)
#SBATCH -t 00:12:00 # max time



 
export MIC_ENV_PREFIX=MIC
export MIC_PPN=1  #NO of MPI tasks for a mic
#export MIC_MY_NSLOTS = 8 #total no of MPI tasks
export MIC_KMP_AFFINITY=compact

ibrun.symm -m "./test.mic ../../inputs/rmat20.gr" –c "./test.out /path/to/inputs/graph.gr"
