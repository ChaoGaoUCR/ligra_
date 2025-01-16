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
    // cout << vertex_[i].first << endl;
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
    // cout << vertex_[i].first << endl;
  }
  return rank;
}

struct BFS_F {
  uintE* Parents;
  BFS_F(uintE* _Parents) : Parents(_Parents) {}
  inline bool update (uintE s, uintE d) { //Update
    if(Parents[d] == UINT_E_MAX) { Parents[d] = s; return 10; }
    else return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic version of Update
    return (CAS(&Parents[d],UINT_E_MAX,s));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (uintE d) { return (Parents[d] == UINT_E_MAX); } 
};

template <class vertex>
uintE* root_compute(graph<vertex>& GA, uintE start) {
    long n = GA.n;
  uintE* Parents = newA(uintE,n);
  parallel_for(long i=0;i<n;i++) Parents[i] = UINT_E_MAX;
  Parents[start] = start;
  vertexSubset Frontier(n,start); //creates initial frontier
  while(!Frontier.isEmpty()){ //loop until frontier is empty
    vertexSubset output = edgeMap(GA, Frontier, BFS_F(Parents));    
    Frontier.del();
    Frontier = output; //set new frontier
  } 
  Frontier.del();
//   free(Parents);
    return Parents;
}

template<class vertex>
uintE** run_10_query(std::vector<uintE> root__, graph<vertex>&G){
  uintE** query_result = newA(uintE*, root__.size());
  for (size_t i = 0; i < root__.size(); i++)
  {
    startTime();
    query_result[i] = root_compute(G,root__[i]);
    nextTime("Running 10 queries time"); 
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
            if (query_result[i][j] != UINT_E_MAX)
            {
                // result nodeid BFS value -> parent || nodeid edge: parent nodeid -> nodeid
                // auto edge = std::make_pair(query_result[i][j], j);
                edge_set.insert(std::make_pair(query_result[i][j], j));
                out_flag[query_result[i][j]] = true;
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
    // G.transpose();
    for (auto i = 0; i < 10; i++)
    {
        for (auto j = 0; j < G.n; j++)
        {
            if (query_result_in[i][j] != UINT_E_MAX)
            {
                // result nodeid BFS value -> parent || nodeid edge: parent nodeid -> nodeid
                // auto edge = std::make_pair(query_result[i][j], j);
                edge_set.insert(std::make_pair(j, query_result_in[i][j]));
                in_flag[query_result_in[i][j]] = true;
            }
        }
    
    }
    for (auto i = 0; i < G.n; i++)
    {
        if (!in_flag[i])
        {
            if (G.V[i].getInDegree() > 0)
            {
                // auto out_node = G.V[i].getOutNeighbor(0);
                // edge_set.insert(std::make_pair(i, out_node));
                auto in_node = G.V[i].getInNeighbor(0);
                edge_set.insert(std::make_pair(in_node, i));
            }
            
        }
    }
    printf("Core Graph Edge Number is %ld\n", edge_set.size());
    free(query_result);
    ofstream of(prefix + ".core_edge_10s");
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