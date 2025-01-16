#include "ligra.h"
#include "vector"
#include "set"
using namespace std;

bool sortByLargerSecondElement(const pair<long, long> &a, const pair<long, long> &b) {
  return (a.second > b.second);
}

template<class vertex>
std::vector<uintE> rank_10_in(graph<vertex>&G){
  std::vector<uintE> rank;
  rank.reserve(10);
  std::vector<pair<uintE, uintE>> vertex_;
  vertex_.reserve(G.n);
  for(uintE i =0; i< G.n; i++) vertex_.emplace_back(std::make_pair(i, G.V[i].getInDegree()));
  std::sort(vertex_.begin(), vertex_.end(), sortByLargerSecondElement);
  for (size_t i = 0; i < 10; i++)
  {
    rank.emplace_back(vertex_[i].first);
    cout << vertex_[i].first << endl;
  }
  return rank;
}

template<class vertex>
std::vector<uintE> rank_10_out(graph<vertex>&G){
  std::vector<uintE> rank;
  rank.reserve(10);
  std::vector<pair<uintE, uintE>> vertex_;
  vertex_.reserve(G.n);
  for(uintE i =0; i< G.n; i++) vertex_.emplace_back(std::make_pair(i, G.V[i].getOutDegree()));
  std::sort(vertex_.begin(), vertex_.end(), sortByLargerSecondElement);
  for (size_t i = 0; i < 10; i++)
  {
    rank.emplace_back(vertex_[i].first);
    cout << vertex_[i].first << endl;
  }
  return rank;
}

struct BF_F {
  uintE* ShortestPathLen;
  int* Visited;
  BF_F(uintE* _ShortestPathLen, int* _Visited) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited) {}
  inline bool update (uintE s, uintE d) { //Update ShortestPathLen if found a shorter path
    uintE edgeLen = (s+d)%32 + 1;
    uintE newDist = ShortestPathLen[s] + edgeLen;
    if(ShortestPathLen[d] > newDist) {
      ShortestPathLen[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
    }
    return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic Update
    uintE edgeLen = (s+d)%32 + 1;
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
    vertexSubset output = edgeMap(GA, Frontier, BF_F(ShortestPathLen,Visited), GA.m/10, dense_forward);
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
uintE** run_10_query(std::vector<uintE> root__, graph<vertex>&G){
  uintE** query_result = newA(uintE*, root__.size());
  for (size_t i = 0; i < root__.size(); i++)
  {
    query_result[i] = root_compute(G,root__[i]);
  }
  
  return query_result;
}

template<class vertex>
void sampling_edge(graph<vertex> &G, string prefix){
    std::vector<uintE> rank = rank_10_out(G);
    std::vector<uintE> rank_in = rank_10_in(G);
    uintE** query_result = run_10_query(rank, G);
    G.transpose();
    uintE** query_result_in = run_10_query(rank_in, G);
    G.transpose();
    bool* out_flag = newA(bool, G.n);
    bool* in_flag = newA(bool, G.n);
    parallel_for (intE i = 0; i < G.n; i++)
    {
        out_flag[i] = false;
        in_flag[i] = false;
    }

    std::set<std::pair<intE, intE>> edge_set;
    for (auto i = 0; i < 10; i++)
    {
        for (auto j = 0; j < G.n; j++)
        {
          intE degreeV = G.V[j].getOutDegree();
            if (query_result[i][j] != 65536)
            {
            intE src = j;
            for (int k = 0; k < degreeV; k++){
              intE dst = G.V[j].getOutNeighbor(k);
              intE edgeLen = (src + dst) % 32 + 1;
              if(query_result[i][src] + edgeLen == query_result[i][dst]){
                if (G.edgecheck(j, dst))
                {
                  edge_set.insert(std::make_pair(j, dst));
                  out_flag[src] = 1;
                  in_flag[dst] = 1;              
                }
              }
            }
            }
        }
    }

    // G.transpose();
    for (auto i = 0; i < 10; i++)
    {
        for (auto j = 0; j < G.n; j++)
        {
          intE dst = j;
            if (query_result_in[i][dst] != 65536)
            {
              intE degreeV = G.V[j].getInDegree();
              for (int k = 0; k < degreeV; k++){
                intE src = G.V[j].getInNeighbor(k);
                intE edgeLen = (src + dst) % 32 + 1;
                if(query_result_in[i][src] + edgeLen == query_result_in[i][j]){
                  if (G.edgecheck(src, j))
                  {
                    edge_set.insert(std::make_pair(src, dst));
                    in_flag[dst] = 1;
                    out_flag[src] = 1;              
                  }
                  }
                }
              }
        }
    }
    for (auto i = 0; i < G.n; i++)
    {
        if (!out_flag[i])
        {
            if (G.V[i].getOutDegree() > 0)
            {
                auto out_node = G.V[i].getOutNeighbor(0);
                edge_set.insert(std::make_pair(i, out_node));
            }
            
        }
    }    
    for (auto i = 0; i < G.n; i++)
    {
        if (!in_flag[i])
        {
            if (G.V[i].getInDegree() > 0)
            {
                auto in_node = G.V[i].getInNeighbor(0);
                edge_set.insert(std::make_pair(in_node, i));
            }
            
        }
    }
    printf("Core Graph Edge Number is %ld\n", edge_set.size());
    free(query_result);
    ofstream of(prefix + ".core_edge_10s_sssp");
    for(auto edge : edge_set){
        of << edge.first << " " << edge.second << endl;
    }
    of.close();
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
    long n = GA.n;
    char* iFile = P.getArgument(0);
    std::string outFile = std::string(iFile) + ".core";
    sampling_edge(GA, outFile);
}