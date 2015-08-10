#include <mpi.h>
#include <omp.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <cmath>


#include <list>
#include <vector>
#include <offload.h>
#include "../../include/FileGraph_xeon.h"
//#include "../../include/FileGraph.h"

using namespace std;

#define MASTER 0
#define TAG_HELLO 4
#define TAG_TEST 5
#define TAG_TIME 6

__declspec(target(mic))
static const double alpha = (0.15);
static const double TOLERANCE = 0.001;
using namespace std;
struct Node{

  Node(){rank = 0; residual = 0;}
  float rank;
  float residual;
  unsigned s_index;
  unsigned numOutEdges;
};



__declspec( target (mic))
void is_it_on_coprocessor()
{

  #ifdef __MIC__

    printf("yes, it is\n");

  #else

    printf("no, it is not\n");

  #endif

}


void graph_initialize(vector<Node>& local_nodes)
{
  #pragma omp parallel
 {
  int nthreads = omp_get_num_threads();
  cout << "nthreads : " << nthreads << "\n";
  }
}

size_t total_nodes(std::string inputFile)
{
    std::ifstream ifs(inputFile.c_str());
    int  max = 0;
    std::string line;
    while (std::getline(ifs, line))
    {
      std::istringstream iss(line);
      int a, b;
      if(!(iss >>a >> b)) { std::cout << "ERROR in reading file\n"; break;}
      if(max < a)
        max = a;
      if(max < b)
        max = b;
    }


  return max;
}
