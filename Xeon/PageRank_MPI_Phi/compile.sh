mpic++ -std=c++0x -fopenmp  -O3 -vec-report2 -o test.out PageRank_Phi_MPI.cpp
mpic++ -std=c++0x -fopenmp  -O3 -vec-report2 -mmic  -o test.mic PageRank_Phi_MPI.cpp
