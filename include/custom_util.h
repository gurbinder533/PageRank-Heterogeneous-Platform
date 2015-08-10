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
//#include <offload.h>
//#include "../../include/FileGraph.h"


#define MASTER 0
#define TAG_HELLO 4
#define TAG_TEST 5
#define TAG_TIME 6


using namespace std;
struct Node{

  Node(){is_there = false; rank = 0; residual = 0;}
  bool is_there;
  float rank;
  float residual;
  //std::atomic<float> residual;
  std::list<unsigned> out_edges;
};

/*void check_nodes(vector<Node>& local_nodes)
{
  int count = 0;
  for(auto n : local_nodes)
  {
    cout << n.out_edges.size() << "\n";
    ++count;
  }

  cout << "Total nodes " << count << "\n";
}

*/
/*void graph_initialize(vector<Node>& local_nodes)
{
 {

 }
}
*/
/*size_t total_nodes(std::string inputFile)
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
      
      //std::cout << "a : " << a << " b : " << b << "\n";
    }
  

  return max;
}*/
