// Run 5 Query and Use them to determine the boundary
// Input is src, and we will use （node -> src）, （node -> dst） to define the （src -> dst） results

#include "ligra.h"
#include "vector"
#include "set"
using namespace std;

bool sortByLargerSecondElement(const pair<long, long> &a, const pair<long, long> &b) {
  return (a.second > b.second);
}

template<class vertex>
std::vector<uintE> rank_5_in(graph<vertex>&G){
  std::vector<uintE> rank;
  rank.reserve(5);
  std::vector<pair<uintE, uintE>> vertex_;
  vertex_.reserve(G.n);
  for(uintE i =0; i< G.n; i++) vertex_.emplace_back(std::make_pair(i, G.V[i].getInDegree()));
  std::sort(vertex_.begin(), vertex_.end(), sortByLargerSecondElement);
  for (size_t i = 0; i < 100; i++)
  {
    rank.emplace_back(vertex_[i].first);
  }
  return rank;
}

template<class vertex>
std::vector<uintE> rank_5_out(graph<vertex>&G){
  std::vector<uintE> rank;
  rank.reserve(5);
  std::vector<pair<uintE, uintE>> vertex_;
  vertex_.reserve(G.n);
  for(uintE i =0; i< G.n; i++) vertex_.emplace_back(std::make_pair(i, G.V[i].getOutDegree()));
  std::sort(vertex_.begin(), vertex_.end(), sortByLargerSecondElement);
  for (size_t i = 0; i < 100; i++)
  {
    rank.emplace_back(vertex_[i].first);
  }
  return rank;
}

struct BF_F {
  uintE* ShortestPathLen;
  int* Visited;
  BF_F(uintE* _ShortestPathLen, int* _Visited) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited) {}
  inline bool update (uintE s, uintE d) { //Update ShortestPathLen if found a shorter path
    uintE edgeLen = (s+d)%16 + 1;
    uintE newDist = ShortestPathLen[s] + edgeLen;
    if(ShortestPathLen[d] > newDist) {
      ShortestPathLen[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
    }
    return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic Update
    uintE edgeLen = (s+d)%16 + 1;
    uintE newDist = ShortestPathLen[s] + edgeLen;
    return (writeMin(&ShortestPathLen[d],newDist) &&
	    CAS(&Visited[d],0,1));
  }
  inline bool cond (uintE d) { return cond_true(d); }
};

//reset visited vertices
struct BF_Vertex_F {
  int* Visited;
  BF_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (uintE i){
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
uintE* root_compute(graph<vertex>& GA, uintE start) {
  long n = GA.n;
  //initialize ShortestPathLen to "infinity"
  uintE* ShortestPathLen = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = 65536;}
  ShortestPathLen[start] = 0;

  int* Visited = newA(int,n);
  {parallel_for(long i=0;i<n;i++) Visited[i] = 0;}

  vertexSubset Frontier(n,start); //initial frontier

  long round = 0;
  while(!Frontier.isEmpty()){
    if(round == n) {
      //negative weight cycle
      {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = 65536;}
      break;
    }
    vertexSubset output = edgeMap(GA, Frontier, BF_F(ShortestPathLen,Visited), GA.m/5, dense_forward);
    vertexMap(output,BF_Vertex_F(Visited));
    Frontier.del();
    Frontier = output;
    round++;
  }
  Frontier.del(); free(Visited);
  // free(ShortestPathLen);
  return ShortestPathLen;
}


template<class vertex>
uintE** run_5_query(std::vector<uintE> root__, graph<vertex>&G){
  uintE** query_result = newA(uintE*, root__.size());
  for (size_t i = 0; i < root__.size(); i++)
  {
    query_result[i] = root_compute(G,root__[i]);
  }
  
  return query_result;
}

template<class vertex>
void tringle_dst_test(graph<vertex> &G, std::string outFile){
    uintE src = 100;
    std::string outBound = outFile + "." +std::to_string(src) + ".tringle_dst_test.txt";
    std::string outResult = outFile + "." + std::to_string(src) +".result.txt";
    ofstream of(outBound);
    ofstream ofResult(outResult);
    std::vector<uintE> rank = rank_5_out(G);
    uintE** query_result = run_5_query(rank, G);
    uintE* query_result_src = newA(uintE, G.n);
    query_result_src = root_compute(G, src);
    
    for(uintE i = 0; i < G.n; i++){
        uintE dst = i;
        // (hot node -> src) + (hot node -> dst) >= (src -> dst)
        // |(hot node -> src) - (hot node -> dst)| <= (src -> dst)
        uintE correct_result = query_result_src[dst];
        uintE upper_bound = 65535;
        uintE lower_bound = 0;
        for (size_t i = 0; i < 100; i++)
        {
            uintE hotnodeToSrc = query_result[i][src];
            uintE hotnodeToDst = query_result[i][dst];
            // printf("Hot Node %d to Src %d is %d\n", rank[i], src, hotnodeToSrc);
            // printf("Hot Node %d to Dst %d is %d\n", rank[i], dst, hotnodeToDst);
            upper_bound = std::min(upper_bound, hotnodeToSrc + hotnodeToDst);
            if(hotnodeToSrc >= hotnodeToDst){
                lower_bound = std::max(lower_bound, hotnodeToSrc - hotnodeToDst);
                }
            else{
                lower_bound = std::max(lower_bound, hotnodeToDst - hotnodeToSrc);
            }
        }
        of << lower_bound << " " << upper_bound << endl;
        ofResult << correct_result << endl;

        // printf("Correct Result is %d\n", correct_result);
        // printf("Bound is %d  -> %d\n", lower_bound, upper_bound);      
}
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
    long n = GA.n;
    char* iFile = P.getArgument(0);
    std::string outFile = std::string(iFile);
    tringle_dst_test(GA, outFile);
}