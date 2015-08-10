/opt/apps/cuda/5.5/bin/nvcc  -I/path/to/modules/mvapich2-2.1-install/include -arch=sm_20  -c PageRank_CUDA_MPI.cu
/path/to/modules/mvapich2-2.1-install/bin/mpic++ -I../../include -std=c++11 -o PageRank PageRank_CUDA_MPI.cpp PageRank_CUDA_MPI.o -L/path/to/cuda/5.5/lib64/ -lcudart
