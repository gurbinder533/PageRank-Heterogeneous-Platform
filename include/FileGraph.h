#ifndef FILEGRAPH
#define FILEGRAPH

#include "common.h"

class FileGraph{

  public:
    enum {NotAllocated, AllocatedOnHost, AllocatedOnDevice} memory;
    unsigned readFromFile(string fileName);
    FileGraph(){init();}
    ~FileGraph();

    unsigned init();
    unsigned readFromGR(string fileName);
    unsigned allocOnHost();
    unsigned deallocOnHost();


  public:
    unsigned nnodes, nedges;
    unsigned *numOutGoing, *numInComing, *srcsrc, *psrc, *edgessrcdst;
    unsigned source;

/*    class out_iterator
    {
      unsigned *fgptr;
      unsigned position;

      public:
        out_iterator(unsigned *_fgptr)
        {
          fgptr = _fgptr;
          position = 0;
        }
        out_iterator(unsigned *_fgptr, unsigned _pos)
        {
          fgptr = _fgptr;
          position = _pos;
        }

        out_iterator(const out_iterator& itr)
        {
          fgptr = itr.fgptr;
          position = rhs.position;
        }
        unsigned& operator&()
        {
          return fgptr[position];
        }

        bool operator==(const iterator &rhs)const
        {
          return ((fgptr == rhs.fgptr) && (position == rhs.position));
        }
         bool operator!=(const iterator &rhs)const
        {
          return !((fgptr == rhs.fgptr) && (position == rhs.position));
        }

      out_iterator& operator++(void)
      {
        position += 1;
        return *this;
      }
      out_iterator& operator++(int)
      {
        out_iterator temp{*this};
        this->operator()++;
        return *temp;
      }

    };

    out_iterator begin()
    {
      return out_iterator(edgessrcdst);
    }
    out_iterator end()
    {
      return out_iterator(edgessrcdst, )
    }
  */
};

unsigned FileGraph::init()
{
  numOutGoing = numInComing = srcsrc = psrc = edgessrcdst = nullptr;
  nnodes = nedges = 0;
  source = 0;

  return 0;
}

FileGraph::~FileGraph(){
  deallocOnHost();
}
unsigned FileGraph::allocOnHost()
{
  edgessrcdst = (unsigned int*)malloc((nedges+1) * sizeof(unsigned int));

  psrc = (unsigned int *)calloc(nnodes+1, sizeof(unsigned int));
  psrc[nnodes] = nedges;

  numOutGoing = (unsigned int *)calloc(nnodes, sizeof(unsigned int));
  numInComing = (unsigned int *)calloc(nnodes, sizeof(unsigned int));
  srcsrc = (unsigned int *)malloc(nnodes * sizeof(unsigned int));

  memory = AllocatedOnHost;
  return 0;
}

unsigned FileGraph::deallocOnHost()
{
  free(numOutGoing);
  free(numInComing);
  free(srcsrc);
  free(psrc);
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
  int _MAP_FLAG = MAP_PRIVATE;

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

  //print some stuff
  std::cout << "Nodes : " << numNodes << " , " << "Edges : " << nedges << "\n";
  allocOnHost();

  for(unsigned ii = 0; ii < nnodes; ++ii)
  {
    srcsrc[ii] = ii;
    if(ii > 0)
    {
      psrc[ii] = le64toh(outIdx[ii - 1]) + 1;
      numOutGoing[ii] = le64toh(outIdx[ii]) - le64toh(outIdx[ii - 1]);
    }
    else{
      psrc[ii] = 1;
      numOutGoing[ii] = le64toh(outIdx[0]);
    }

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
      ++numInComing[dst];
    }

  }
  ifs.close();

  return 0;
}
#endif
