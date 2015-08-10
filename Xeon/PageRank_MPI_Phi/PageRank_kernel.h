/***
 * Author: Gurbinder Gill
 * Email : gill@cs.utexas.edu
 */

__declspec(target(mic))
void PageRank_init(vector<Node>& local_nodes, vector<unsigned>& out_edges, unsigned local_chunk_size, unsigned global_chunk_size, unsigned total_nodes, int hosts, int iterations)
{
  vector<float> delta_send(total_nodes, 0.0);
  vector<float> delta_recv(total_nodes, 0.0);

  int my_id;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

  unsigned int nThreads = sysconf(_SC_NPROCESSORS_ONLN) - 4;
  if(my_id == 0)
  {
    std::cout << " THREADS : " << nThreads << "\n";
  }
  for(int itr = 0; itr < iterations; ++itr)
  {


    if(my_id == 0)
      std::cout << " Iterations No. : " << itr << "\n";

    //#pragma omp parallel for
    #pragma omp parallel for schedule(static,1000) num_threads(nThreads)
    for(unsigned i = 0; i < local_chunk_size; ++i)
    {
      Node n = local_nodes[i];
      float old_residual = n.residual;
      local_nodes[i].residual = 0.0;
      if( itr > 0)
      {
        local_nodes[i].rank += old_residual;
      }

      if(n.numOutEdges != 0)
      {
        float delta = old_residual*alpha/n.numOutEdges;

        for(int ii = n.s_index; ii < (n.s_index + n.numOutEdges); ++ii)
        {
          #pragma omp atomic
          delta_send[out_edges[ii]] += delta;
        }
      }
    }


    /*
       if(my_id == 0 && itr == 1)
       {
       for(int k = global_chunk_size; k < 10 + global_chunk_size ; ++k)
       {
       std::cout << " - > " << k << " : " << delta_send[k] << "\n"; 
       }
       }
       */

    // Using MPI_AllToAll
    int status;
    status = MPI_Alltoall(&delta_send.front(),global_chunk_size, MPI_FLOAT, &delta_recv.front(), global_chunk_size, MPI_FLOAT, MPI_COMM_WORLD);

    if(status != MPI_SUCCESS)
    {
      cout << "MPI_ALLToAll failed\n";
      exit(0);
    }


    //Apply updates

    //#pragma omp parallel for schedule(dynamic,1) 
    #pragma omp parallel for schedule(static,1000) num_threads(nThreads)
    for(unsigned i = 0; i < local_chunk_size; ++i)
    {
      unsigned k = 0;
      float temp_residual = 0.0;
      for(unsigned j = 0; j < hosts; ++j)
      {
        k  = i + global_chunk_size*j;
        temp_residual += delta_recv[k];
        delta_send[k] = 0.0;
      }

      local_nodes[i].residual = temp_residual;
    }

    /*
       if(my_id == 1 && itr == 1)
       {
       for(int k = 0; k < 10 ; ++k)
       {
       std::cout << " - > " << k << " : " << delta_recv[k] << "\n"; 
       }
       }
       */

  }

}

