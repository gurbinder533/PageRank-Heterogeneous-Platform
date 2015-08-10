/***
 * Author: Gurbinder Gill
 * Email : gill@cs.utexas.edu
 */

#include "custom_util.h"
#include "PageRank_kernel.h"
#include <ctime>
#include <sys/time.h> 

#define MASTER 0
#define TAG_HELLO 4
#define TAG_TEST 5
#define TAG_TIME 6

unsigned global_chunk_size;
unsigned readFile(string fileName, vector<Node>& local_nodes, vector<unsigned>& local_OutEdges)
{
  int hosts, my_id;
  global_chunk_size = 0;

  MPI_Comm_size(MPI_COMM_WORLD, &hosts);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

  FileGraph fg;
  fg.readFromGR(fileName, my_id, hosts);

  int total_nodes = fg.nnodes;

  if(my_id == 0)
  {
    global_chunk_size = ceil((float(total_nodes)/(hosts)));
    for (int dest = 1; dest < hosts; ++dest)
      MPI_Send(&global_chunk_size, 1, MPI_INT, dest, MASTER, MPI_COMM_WORLD);
  }
  else {
    MPI_Recv(&global_chunk_size, 1, MPI_INT, 0, MASTER, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  cout << "[ " << my_id << " ]" << " Chunk Size : " << global_chunk_size << "\n";

  int start_index = my_id*global_chunk_size;
  int end_index = start_index + global_chunk_size;
  if(end_index > total_nodes)
  {
    end_index = total_nodes;
  }

  cout << "[ " << my_id << " ]" << " start : " << start_index << " end : " << end_index << "\n";
  //local_nodes.resize(end_index - start_index);
  int local_number_nodes = (end_index - start_index);

  unsigned edge_index = 0;
  for(auto i = 0; i < local_number_nodes; ++i)
  {
    Node n;
    n.rank = 1 - alpha;
    n.residual =  (1 - alpha);
    n.s_index = edge_index;

    int out_edges_num = fg.numOutGoing[i];
    n.numOutEdges = out_edges_num;
    for(int j = 0; j < out_edges_num; ++j)
    {
      auto edgeIndex = edge_index + j ; //fg.psrc[i] + j;
      auto dst =  fg.edgessrcdst[edgeIndex];
      local_OutEdges.push_back(dst);
    }
    edge_index += out_edges_num ;
    local_nodes.push_back(n);
  }

  std::cout << "[ "<<my_id <<" ] Done Constructing Graph \n";
  return total_nodes;
}


int main(int argc, char* argv[])
{

//Initialize MPI
  if (MPI_Init(&argc, &argv) != MPI_SUCCESS){
    printf ("MPI Initiaizing failed");
    return(-1);
  }
  int namelen;
  char name[MPI_MAX_PROCESSOR_NAME];
  int i,world_size, id,remote_id;

  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Get_processor_name (name, &namelen);

  MPI_Status stat;
  // Read graph on host 0
  if(argc < 2)
  {
    std::cout << "Usage : ./a.out fileName (gr format)\n";
    exit(1);
  }
  std::string fileName = argv[1];;
  std::cout << fileName << "\n";

  // my local nodes in a vector
  vector<Node> local_nodes;
  vector<unsigned> local_edges;
  std::cout << "READING FILE \n";
  unsigned total_nodes = readFile(fileName, local_nodes, local_edges);

  if(id == 0)
  {
    std::cout << " TOTAL hosts : " << world_size << "\n";
    std::cout << " Total Nodes : " << total_nodes << "\n";
    //is_it_on_coprocessor();
  }

  unsigned local_chunk_size = local_nodes.size();
  int iterations = 60;

    struct timeval tim;  
    gettimeofday(&tim, NULL);  
    double t1=tim.tv_sec+(tim.tv_usec/1000000.0);  

  PageRank_init(local_nodes, local_edges, local_chunk_size, global_chunk_size, total_nodes, world_size, iterations);

    gettimeofday(&tim, NULL);  
    double t2=tim.tv_sec+(tim.tv_usec/1000000.0);  

  if(id == 0)
    printf("%.6lf seconds elapsed\n", t2-t1); 

  if(id == 0)
  {
      for(int i = 0; i < 10; ++i)
      {
        std::cout << " R : " << i << " : " << local_nodes[i].rank << "\n";
      }
  }

  MPI_Finalize();
}




