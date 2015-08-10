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


#define MASTER 0
#define TAG_HELLO 4
#define TAG_TEST 5
#define TAG_TIME 6

static const double alpha = (1.0 - 0.85);
static const double TOLERANCE = 0.1;
using namespace std;
struct Node{

  Node(){rank = 0; residual = 0;}
  float rank;
  float residual;
  unsigned s_index;
  unsigned numOutEdges;
};

