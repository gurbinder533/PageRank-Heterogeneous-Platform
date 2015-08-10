/***
 * Author: Gurbinder Gill
 * Email : gill@cs.utexas.edu
 */
#include <iostream>
#include <stdio.h>
#include <cuda.h>
#include <ctime>
#include <mpi.h>
#include <vector>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include "../../include/custom_util_cuda.h"

using namespace std;

__global__ void add1kernel(int* input)
{
  int tid = blockIdx.x*blockDim.x +  threadIdx.x;
  input[tid] = input[tid] + 1;
}

__global__ void checkNodes_kernel(Node* local_nodes, unsigned num_nodes)
{
  int index = threadIdx.x + blockDim.x*blockIdx.x;
  if(index < num_nodes)
  {
    local_nodes[index].rank = 1;
    printf("on device : %d\n", local_nodes[index].rank);
  }
}


__global__ void PageRank_init(Node* local_nodes,unsigned* out_edges, unsigned num_nodes, float* delta_send, int host_id)
{
  int index = threadIdx.x + blockDim.x*blockIdx.x;

  if(index < num_nodes)
  {
    Node n = local_nodes[index];
    float old_residual = n.rank*alpha;

    if(n.numOutEdges != 0)
    {
      float delta = old_residual/n.numOutEdges;
      for(int ii = n.s_index; ii < (n.s_index + n.numOutEdges); ++ii)
      {
        atomicAdd(&delta_send[out_edges[ii]], delta);
      }
    }
  }

}


__global__ void PageRank(Node* local_nodes,unsigned* out_edges, unsigned num_nodes, float* delta_send, int host_id)
{
  int index = threadIdx.x + blockDim.x*blockIdx.x;

  if(index < num_nodes)
  {
    Node n = local_nodes[index];
    float old_residual = n.residual;
    n.residual = 0.0;

    local_nodes[index].rank += old_residual;

    if(n.numOutEdges !=0)
    {
      float delta = old_residual*alpha/n.numOutEdges;
      for(int ii = n.s_index; ii < (n.s_index + n.numOutEdges); ++ii)
      {
        atomicAdd(&delta_send[out_edges[ii]], delta);
      }
    }
  }

}

__global__ void ApplyDelta(Node* local_nodes, float* delta_recv, float* delta_send, unsigned total_nodes, unsigned global_chunk_size, unsigned hosts, unsigned my_id)
{
  int index = threadIdx.x + blockDim.x*blockIdx.x;
  if(index < global_chunk_size)
  {
    int j = 0;
    float temp_residual = 0.0;
    for(int i = 0; i < hosts; ++i)
    {
        j = index + global_chunk_size*i;
        temp_residual += delta_recv[j];
        delta_send[j] = 0.0;
    }
    local_nodes[index].residual = temp_residual;
  }
}

__global__ void finish_pageRank(Node* local_nodes, unsigned total_nodes, unsigned local_chunk_size)
{

  int index = threadIdx.x + blockDim.x*blockIdx.x;
  if(index < local_chunk_size)
  {
    local_nodes[index].rank = local_nodes[index].rank/total_nodes;
  }
}

void cudaFunction(unsigned total_nodes,unsigned global_chunk_size, vector<Node>& local_nodes, vector<unsigned>& out_edges, int iterations)
{

  int local_chunk_size = local_nodes.size();

  std::cout << "local chunk size :" << local_chunk_size << "\n";
  std::cout << "Global chunk size :" << global_chunk_size << "\n";

  int my_id;
  int hosts;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  MPI_Comm_size(MPI_COMM_WORLD, &hosts);

  std::cout << " ID : " << my_id << "\n";
  Node* device_local_nodes;

  cudaMalloc(&device_local_nodes, sizeof(Node)*local_chunk_size);
  cudaMemcpy(device_local_nodes, &local_nodes.front(), sizeof(Node)*local_chunk_size, cudaMemcpyHostToDevice);

  int num_outEdges = out_edges.size();
  unsigned* device_out_edges;

  cudaMalloc(&device_out_edges, sizeof(unsigned)*num_outEdges);
  cudaMemcpy(device_out_edges, &out_edges.front(), sizeof(unsigned)*num_outEdges, cudaMemcpyHostToDevice);


  float *device_delta_send, *device_delta_recv;
  unsigned total_new = hosts*global_chunk_size;
  cudaMalloc(&device_delta_send, sizeof(float)*total_new);
  cudaMalloc(&device_delta_recv, sizeof(float)*total_new);
  float *host_zero;
  host_zero = (float*)calloc(total_new, sizeof(float));

  cudaMemcpy(device_delta_send, host_zero, sizeof(float)*total_new, cudaMemcpyHostToDevice);
  cudaMemcpy(device_delta_recv, host_zero, sizeof(float)*total_new, cudaMemcpyHostToDevice);



  // Bulk synchronous PageRank : STARTS

  int threads = 512;
  int blocks = local_chunk_size/threads + (local_chunk_size%threads == 0 ? 0 : 1);

  cout << "Blocks : " << blocks << "\n";

  ///////////////// Intialize Graph ///////////////////////

  if(my_id == 0)
    std::cout << " PAGE RANK : Initialization Phase Starts\n";
  PageRank_init<<<blocks, threads>>>(device_local_nodes, device_out_edges, local_chunk_size, device_delta_send,my_id );
  cudaThreadSynchronize();

  // Using MPI_AllToAll
  int status;
  status = MPI_Alltoall(device_delta_send,global_chunk_size, MPI_FLOAT, device_delta_recv, global_chunk_size, MPI_FLOAT, MPI_COMM_WORLD);

  if(status != MPI_SUCCESS)
  {
    cout << "MPI_ALLToAll failed\n";
    exit(0);
  }


  // Apply received delta values.
  ApplyDelta<<<blocks, threads>>>(device_local_nodes, device_delta_recv, device_delta_send, total_nodes, global_chunk_size, hosts, my_id );
  cudaThreadSynchronize();


  if(my_id == 0)
    std::cout << " PAGE RANK : Initialization Phase Ends\n";
  //////////////////////////////////////////////////////


  int iterations_fixed = 60;
  clock_t start_pg, end_pg;
  start_pg = clock();

  // Different timers
  //clock_t pageRank_start_t, mpi_start_t, apply_start_t;
  //double pageRank_dur_t = 0, mpi_dur_t = 0, apply_dur_t = 0;

  for(int itr = 0; itr < iterations_fixed; ++itr)
  {
    if(my_id == 0)
      std::cout << "Iteration no. : " << itr <<"\n";

    //pageRank_start_t  = clock();

    PageRank<<<blocks, threads>>>(device_local_nodes, device_out_edges, local_chunk_size, device_delta_send,my_id );
    cudaThreadSynchronize();

    //pageRank_dur_t +=  double(clock() - pageRank_start_t) / CLOCKS_PER_SEC;

    // CUDA Aware MPI
    int status;

    //mpi_start_t = clock();

    // Using MPI_AllToAll
    status = MPI_Alltoall(device_delta_send,global_chunk_size, MPI_FLOAT, device_delta_recv, global_chunk_size, MPI_FLOAT, MPI_COMM_WORLD);

    if(status != MPI_SUCCESS)
    {
      cout << "MPI_ALLToAll failed\n";
      exit(0);
    }

    //mpi_dur_t +=  double(clock() - mpi_start_t) / CLOCKS_PER_SEC;


    //apply_start_t = clock();

    // Apply received delta values.
    ApplyDelta<<<blocks, threads>>>(device_local_nodes, device_delta_recv, device_delta_send, total_nodes, global_chunk_size, hosts, my_id );
    cudaThreadSynchronize();

    //apply_dur_t +=  double(clock() - apply_start_t) / CLOCKS_PER_SEC;

    // PageRank : ENDS
  }

  end_pg = clock();
  double elapsed_secs = double(end_pg - start_pg) / CLOCKS_PER_SEC;

  if(my_id == 0)
  {
    std::cout << "Total Time for " << iterations << " : " << elapsed_secs << " sec.\n";
   // std::cout << "Total Time for  PageRank routine: " << pageRank_dur_t << " sec.\n";
   // std::cout << "Total Time for  MPI routine: " << mpi_dur_t << " sec.\n";
   // std::cout << "Total Time for  Apply routine: " << apply_dur_t << " sec.\n";

    Node * host_check_nodes;
    host_check_nodes = (Node*)malloc(sizeof(Node)*local_chunk_size);
    cudaMemcpy(host_check_nodes, device_local_nodes, sizeof(Node)*local_chunk_size, cudaMemcpyDeviceToHost);

    for(int i = 0; i < 10; ++i)
    {
        cout << "\n R : " << i << " : " << host_check_nodes[i].rank << "\n";
    }
  }


}

