#ifndef FILEGRAPH
#define FILEGRAPH

#include "common.h"
#include <math.h>

class FileGraph{

  public:
    enum {NotAllocated, AllocatedOnHost, AllocatedOnDevice} memory;
    unsigned readFromFile(string fileName);
    FileGraph(){init();}
    ~FileGraph();

    unsigned init();
    unsigned readFromGR(string fileName, int, int);
    unsigned readFromGR(string fileName);
    unsigned allocOnHost();
    unsigned allocOnHost(unsigned);
    unsigned allocOnHost_edges(unsigned);
    unsigned deallocOnHost();


  public:
    int id;
    unsigned nnodes, nedges;
    unsigned *numOutGoing, *numInComing, *psrc, *edgessrcdst;
    unsigned source;

};

unsigned FileGraph::init()
{
  numOutGoing = numInComing = psrc = edgessrcdst = nullptr;
  nnodes = nedges = 0;
  source = 0;

  return 0;
}

FileGraph::~FileGraph(){
  deallocOnHost();
}
unsigned FileGraph::allocOnHost()
{
  edgessrcdst = (unsigned int*)calloc((nedges+1),  sizeof(unsigned int));
  if(edgessrcdst == NULL)
    printf("FFFFFFFFFfailed to allocate edgessrcdst\n"); 

  psrc = (unsigned int *)calloc(nnodes+1, sizeof(unsigned int));
  if(psrc == NULL)
    printf("FFFFFFFFFFFfailed to allocate psrc\n"); 

  psrc[nnodes] = nedges;

  numOutGoing = (unsigned int *)calloc(nnodes, sizeof(unsigned int));
  if(numOutGoing == NULL)
    printf("FFFFFFFFFFfailed to allocate numOutGoing\n"); 
  //numInComing = (unsigned int *)calloc(nnodes, sizeof(unsigned int));
  //srcsrc = (unsigned int *)malloc(nnodes * sizeof(unsigned int));

  memory = AllocatedOnHost;
  return 0;
}

unsigned FileGraph::allocOnHost(unsigned local_chunk_size)
{

  psrc = (unsigned int *)calloc(local_chunk_size+1, sizeof(unsigned int));
  if(psrc == NULL)
    printf("failed to allocate psrc\n"); 

  psrc[local_chunk_size] = 0 ; //local_chunk_size;

  numOutGoing = (unsigned int *)calloc(local_chunk_size, sizeof(unsigned int));
  if(numOutGoing == NULL)
    printf("failed to allocate numOutGoing\n"); 
  //numInComing = (unsigned int *)calloc(nnodes, sizeof(unsigned int));
  //srcsrc = (unsigned int *)malloc(nnodes * sizeof(unsigned int));

  return 0;
}

unsigned FileGraph::allocOnHost_edges(unsigned local_edges)
{
  edgessrcdst = (unsigned int*)calloc((local_edges+1) , sizeof(unsigned int));
  if(edgessrcdst == NULL)
    printf("xxxxxxxxxxxxxxxxxxxxxxxxfailed to allocate edgessrcdst\n"); 

  return 0;
}


unsigned FileGraph::deallocOnHost()
{
  if(numOutGoing != NULL)
    free(numOutGoing);
  //free(numInComing);
  //free(srcsrc);
  if(psrc != NULL)
    free(psrc);
  if(edgessrcdst != NULL)
    free(edgessrcdst);

  return 0;
}

unsigned FileGraph::readFromGR(string fileName)
{
  std::cout << "fileName : " << fileName <<"\n";
  std::ifstream ifs;
  ifs.open(fileName.c_str());

  int FileDesc = open(fileName.c_str(), O_RDONLY);
  if(FileDesc == -1)
  {
    cout << "FileGraph:: unable to open the file" << fileName <<"\n";
    return 1;
  }

  struct stat buf;
  int f = fstat(FileDesc, &buf);
  if(f == -1)
  {
    cout << "FileGraph:: unable to fstat" << fileName << "\n";
    exit(1);
  }

  size_t TotalLength = buf.st_size;
  int _MAP_FLAG = MAP_SHARED; //MAP_PRIVATE;

  std::cout << "TOTAL LENGHT : " << TotalLength/1024 << " MB \n";
  void* m = mmap64(0, TotalLength, PROT_READ, _MAP_FLAG, FileDesc, 0);
  if(m == MAP_FAILED)
  {
    m = 0;
    cout << "FileGraph:: mmap failed.\n";
    exit(1);
  }

  uint64_t* fatPtr = (uint64_t*)m;
  __attribute__((unused)) uint64_t version = le64toh(*fatPtr++);
  assert(version == 1);
  uint64_t sizeEdgeTy = le64toh(*fatPtr++);
  uint64_t numNodes = le64toh(*fatPtr++);
  uint64_t numEdges = le64toh(*fatPtr++);
  uint64_t *outIdx = fatPtr;
  fatPtr += numNodes;
  uint32_t *fatPtr32 = (uint32_t*)fatPtr;
  uint32_t *outs = (uint32_t*)fatPtr32;
  fatPtr32 += numEdges;

  // This is for edge data, but we don't need that
  if(numEdges % 2) 
  {
    fatPtr32 += 1;
  }

  nnodes = numNodes;
  nedges = numEdges;

  //print some stuff
  std::cout << "Nodes : " << numNodes << " , " << "Edges : " << nedges << "\n";
  allocOnHost();
  std::cout << "Allocate success \n";


  for(unsigned ii = 0; ii < nnodes; ++ii)
  {
    //srcsrc[ii] = ii;
    if(ii > 0)
    {
      psrc[ii] = le64toh(outIdx[ii - 1]) + 1;
      numOutGoing[ii] = le64toh(outIdx[ii]) - le64toh(outIdx[ii - 1]);
    }
    else{
      psrc[ii] = 1;
      numOutGoing[ii] = le64toh(outIdx[0]);
    }

    //std::cout << "normal -> " << ii << " : " <<psrc[ii] <<"\n";
    for(unsigned jj = 0; jj < numOutGoing[ii]; ++jj)
    {
      unsigned edgeIndex = psrc[ii] + jj;
      unsigned dst = le32toh(outs[edgeIndex - 1]);
      if(dst >= nnodes )
      {
        std::cout << "Invalid Edge\n";
        exit(1);
      }

      edgessrcdst[edgeIndex] = dst;
      //++numInComing[dst];
    }

  }

  ifs.close();

  return 0;
}

unsigned FileGraph::readFromGR(string fileName, int my_id, int hosts)
{
  std::cout << "fileName : " << fileName <<"\n";
  std::ifstream ifs;
  ifs.open(fileName.c_str());

  int FileDesc = open(fileName.c_str(), O_RDONLY);
  if(FileDesc == -1)
  {
    cout << "FileGraph:: unable to open the file" << fileName <<"\n";
    return 1;
  }

  struct stat buf;
  int f = fstat(FileDesc, &buf);
  if(f == -1)
  {
    cout << "FileGraph:: unable to fstat" << fileName << "\n";
    exit(1);
  }

  size_t TotalLength = buf.st_size;
  int _MAP_FLAG = MAP_SHARED; //MAP_PRIVATE;

  void* m = mmap(0, TotalLength, PROT_READ, _MAP_FLAG, FileDesc, 0);
  if(m == MAP_FAILED)
  {
    m = 0;
    cout << "FileGraph:: mmap failed.\n";
    exit(1);
  }

  uint64_t* fatPtr = (uint64_t*)m;
  __attribute__((unused)) uint64_t version = le64toh(*fatPtr++);
  assert(version == 1);
  uint64_t sizeEdgeTy = le64toh(*fatPtr++);
  uint64_t numNodes = le64toh(*fatPtr++);
  uint64_t numEdges = le64toh(*fatPtr++);
  uint64_t *outIdx = fatPtr;
  fatPtr += numNodes;
  uint32_t *fatPtr32 = (uint32_t*)fatPtr;
  uint32_t *outs = (uint32_t*)fatPtr32;
  fatPtr32 += numEdges;

  // This is for edge data, but we don't need that
  if(numEdges % 2) 
  {
    fatPtr32 += 1;
  }

  nnodes = numNodes;
  nedges = numEdges;

  unsigned global_chunk_size = 1 + ((nnodes - 1) / hosts); //ceil(nnodes/hosts);

  unsigned start_index = my_id*global_chunk_size;
  unsigned end_index = start_index + global_chunk_size;

  if(end_index > nnodes)
  {
    end_index = nnodes;
  }

  unsigned local_chunk_size = (end_index - start_index);

  //print some stuff
  std::cout << "Nodes : " << numNodes << " , " << "Edges : " << nedges << "\n";
  std::cout << "Local chuck size  : " << my_id << " : "<< local_chunk_size  << " , " << "Global chunk size : " << global_chunk_size << "\n";
  cout << "[ " << my_id << " ]" << " start : " << start_index << " end : " << end_index << "\n";

  allocOnHost(local_chunk_size);

  unsigned total_local_edges = 0;
  unsigned kk = 0;
  for(unsigned ii = start_index; (ii < end_index) && (kk < local_chunk_size); ++ii, ++kk)
  {
    if(ii > 0)
    {
      psrc[kk] = le64toh(outIdx[ii - 1]) + 1;
      numOutGoing[kk] = le64toh(outIdx[ii]) - le64toh(outIdx[ii - 1]);
    }
    else{
      psrc[kk] = 1;
      numOutGoing[kk] = le64toh(outIdx[0]);
    }

    total_local_edges += numOutGoing[kk];
  }

  allocOnHost_edges(total_local_edges);

  unsigned local_edge_index = 0;
  unsigned l_edge ; 
  unsigned edgeIndex ; 
  unsigned dst ;
  
  kk = 0;
  for( unsigned ii = start_index; (ii < end_index) && (kk < local_chunk_size); ++ii, ++kk)
  {
    for(unsigned jj = 0; jj < numOutGoing[kk]; ++jj)
    {
      l_edge = local_edge_index + jj;
      edgeIndex = psrc[kk] + jj;
      dst = le32toh(outs[edgeIndex - 1]);
      edgessrcdst[l_edge] = dst;
    }
   local_edge_index += (numOutGoing[kk]);
  }

  std::cout << "Edges Assigned Successfully\n";
  ifs.close();

  return 0;
}
#endif
