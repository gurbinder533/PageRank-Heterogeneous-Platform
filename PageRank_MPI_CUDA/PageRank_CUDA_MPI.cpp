/***
 * Author: Gurbinder Gill
 * Email : gill@cs.utexas.edu
 */

#include<climits>
#include <ctime>
#include "../../include/FileGraph.h"
#include "../../include/custom_util_cuda.h"

void cudaFunction(unsigned,unsigned,vector<Node>&, vector<unsigned>&, int);
unsigned global_chunk_size;

unsigned readFile(string fileName, vector<Node>& local_nodes, vector<unsigned>& local_OutEdges)
{
  FileGraph fg;
  fg.readFromGR(fileName);

  int total_nodes = fg.nnodes;

  int hosts, my_id;
  global_chunk_size = 0;

  MPI_Comm_size(MPI_COMM_WORLD, &hosts);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  if(my_id == 0)
  {
    global_chunk_size = ceil((float(total_nodes)/(hosts)));
    for (int dest = 1; dest < hosts; ++dest)
      MPI_Send(&global_chunk_size, 1, MPI_INT, dest, MASTER, MPI_COMM_WORLD);
  }
  else {
    MPI_Recv(&global_chunk_size, 1, MPI_INT, 0, MASTER, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  //cout << "[ " << my_id << " ]" << " Chunk Size : " << global_chunk_size << "\n";

  int start_index = my_id*global_chunk_size;
  int end_index = start_index + global_chunk_size;
  if(end_index > total_nodes)
  {
    end_index = total_nodes;
  }

  cout << "[ " << my_id << " ]" << " start : " << start_index << " end : " << end_index << "\n";
  int local_number_nodes = (end_index - start_index);

  unsigned edge_index = 0;
  unsigned max_inDegree_node = 0;
  unsigned max_inDegree = 0;

  unsigned min_inDegree_node = 0;
  unsigned min_inDegree = total_nodes;
  for(auto i = start_index; i < end_index; ++i)
  {
    Node n;
    n.rank = 1 - alpha;
    n.residual = 0.0;
    n.s_index = edge_index;

    int out_edges_num = fg.numOutGoing[i];

    n.numOutEdges = out_edges_num;
    edge_index += out_edges_num;
    for(int j = 0; j < out_edges_num; ++j)
    {
      auto edgeIndex = fg.psrc[i] + j;
      auto dst = fg.edgessrcdst[edgeIndex];
      local_OutEdges.push_back(dst);
      //n.out_edges.push_back(dst);
    }
    local_nodes.push_back(n);
  }

  //CHECKING
  //std::cout << "max inDgree : " << max_inDegree << " of Node : " << max_inDegree_node << "\n";
  //std::cout << "min inDgree : " << min_inDegree << " of Node : " << min_inDegree_node << "\n";
  return total_nodes;
}


int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  int world_size, my_id;

  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

  vector<Node> local_nodes;
  vector<unsigned> local_edges;
/*  if(argc < 3){
    std::cout << " Wrong Arguement. Usage ./pagerank inputfile num_iterations\n";
    exit(1);
  }
  */
  string fileName = argv[1];
  int iterations = 0 ; //atoi(argv[2]);
  unsigned total_nodes = readFile(fileName, local_nodes, local_edges);
  if(my_id == 0)
  {
    std::cout << " TOTAL hosts : " << world_size << "\n";
  }

  cudaFunction(total_nodes, global_chunk_size, local_nodes, local_edges, iterations);

  MPI_Finalize();
}
