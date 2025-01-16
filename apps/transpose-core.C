#define WEIGHTED 1
#include "ligra.h"
#include "vector"
#include "set"
using namespace std;

bool sortByLargerSecondElement(const pair<long, long> &a, const pair<long, long> &b) {
  return (a.second > b.second);
}

template<class vertex>
std::vector<intE> rank_10_in(graph<vertex>&G){
  std::vector<intE> rank;
  rank.reserve(10);
  std::vector<pair<intE, intE>> vertex_;
  vertex_.reserve(G.n);
  for(intE i =0; i< G.n; i++) vertex_.emplace_back(std::make_pair(i, G.V[i].getInDegree()));
  std::sort(vertex_.begin(), vertex_.end(), sortByLargerSecondElement);
  for (size_t i = 0; i < 10; i++)
  {
    rank.emplace_back(vertex_[i].first);
    // cout << vertex_[i].first << endl;
  }
  return rank;
}

template<class vertex>
std::vector<intE> rank_10_out(graph<vertex>&G){
  std::vector<intE> rank;
  rank.reserve(10);
  std::vector<pair<intE, intE>> vertex_;
  vertex_.reserve(G.n);
  for(intE i =0; i< G.n; i++) vertex_.emplace_back(std::make_pair(i, G.V[i].getOutDegree()));
  std::sort(vertex_.begin(), vertex_.end(), sortByLargerSecondElement);
  for (size_t i = 0; i < 10; i++)
  {
    rank.emplace_back(vertex_[i].first);
    // cout << vertex_[i].first << endl;
  }
  return rank;
}

struct BF_F {
  intE* ShortestPathLen;
  int* Visited;
  BF_F(intE* _ShortestPathLen, int* _Visited) : 
    ShortestPathLen(_ShortestPathLen), Visited(_Visited) {}
  inline bool update (intE s, intE d, intE edgeLen) { //Update ShortestPathLen if found a shorter path
    intE newDist = ShortestPathLen[s] + edgeLen;
    if(ShortestPathLen[d] > newDist) {
      ShortestPathLen[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
    }
    return 0;
  }
  inline bool updateAtomic (intE s, intE d, intE edgeLen){ //atomic Update
    intE newDist = ShortestPathLen[s] + edgeLen;
    return (writeMin(&ShortestPathLen[d],newDist) &&
	    CAS(&Visited[d],0,1));
  }
  inline bool cond (intE d) { return cond_true(d); }
};

//reset visited vertices
struct BF_Vertex_F {
  int* Visited;
  BF_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (intE i){
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
intE* root_compute(graph<vertex>& GA, intE start) {
  long n = GA.n;
  //initialize ShortestPathLen to "infinity"
  intE* ShortestPathLen = newA(intE,n);
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
intE** run_10_query(std::vector<intE> root__, graph<vertex>&G){
  intE** query_result = newA(intE*, root__.size());
  parallel_for (size_t i = 0; i < root__.size(); i++)
  {
    query_result[i] = root_compute(G,root__[i]);
  }
  
  return query_result;
}

template<class vertex>
void sampling_edge(graph<vertex> &G, string prefix){
    auto node_test = 1;
    for (size_t i = 0; i < G.V[node_test].getOutDegree(); i++)
    {
      if (G.V[node_test].getOutNeighbor(i) == 224 || G.V[node_test].getOutNeighbor(i) == 331){
      printf("Node %d, OutNeighbor %d, OutWeight %d\n", node_test, G.V[node_test].getOutNeighbor(i), G.V[node_test].getOutWeight(i));}
    }
    std::vector<intE> rank = rank_10_out(G);
    std::vector<intE> rank_in = rank_10_in(G);
    intE** query_result = run_10_query(rank, G);
    G.transpose();
    intE** query_result_in = run_10_query(rank_in, G);
    G.transpose();
    printf("Finish to run 10 queries\n");
    bool* out_flag = newA(bool, G.n);
    bool* in_flag = newA(bool, G.n);
    parallel_for (intE i = 0; i < G.n; i++)
    {
        out_flag[i] = false;
        in_flag[i] = false;
    }

    std::set<tuple<intE, intE, intE>> edge_set;

    bool** edge_flag = newA(bool*, G.n);
    bool** edge_flag_in = newA(bool*, G.n);
    // set all edge to be false at begining
    parallel_for(auto e = 0; e < G.n; e++){
        auto degreeV = G.V[e].getOutDegree();
        auto degreeV_in = G.V[e].getInDegree();
        edge_flag[e] = newA(bool, degreeV);
        edge_flag_in[e] = newA(bool, degreeV_in);
        parallel_for(auto i = 0; i < degreeV; i++){
            edge_flag[e][i] = false;
        }
        parallel_for(auto i = 0; i < degreeV_in; i++){
            edge_flag_in[e][i] = false;
        }
    }
    printf("Start to find core graph edge\n");
    parallel_for(auto i = 0; i < 10; i++){
        parallel_for(auto j = 0; j < G.n; j++){
            auto degreeV = G.V[j].getOutDegree();
            parallel_for(auto edge_ptr = 0; edge_ptr < degreeV; edge_ptr++){
                auto src = j;
                auto dst = G.V[j].getOutNeighbor(edge_ptr);
                auto edgeLen = G.V[j].getOutWeight(edge_ptr);
                if(query_result[i][src] + edgeLen == query_result[i][j]){
                    if (G.edgecheck(src, dst)){
                        out_flag[src] = 1;
                        in_flag[dst] = 1;
                        edge_flag[j][edge_ptr] = true;
                    }
                }
            }
        }
    }
    parallel_for(auto i = 0; i < 10; i++){
        parallel_for(auto j = 0; j < G.n; j++){
            auto degreeV = G.V[j].getInDegree();
            parallel_for(auto edge_ptr = 0; edge_ptr < degreeV; edge_ptr++){
                auto src = G.V[j].getInNeighbor(edge_ptr);
                auto dst = j;
                auto edgeLen = G.V[j].getInWeight(edge_ptr);
                if(query_result_in[i][src] + edgeLen == query_result_in[i][j]){
                    if (G.edgecheck(src, dst)){
                        out_flag[dst] = 1;
                        in_flag[src] = 1;
                        edge_flag_in[j][edge_ptr] = true;
                    }
                }
            }
        }
    }
    printf("Finish to find core graph edge with query results\n");
    for (auto i = 0; i < G.n; i++)
    {
        if (!out_flag[i])
        {
            if (G.V[i].getOutDegree() > 0)
            {
                auto out_node = G.V[i].getOutNeighbor(0);
                // auto wgh = G.V[i].getOutWeight(0);
                edge_flag[i][0] = true;
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
                edge_flag_in[i][0] = true;
                // auto wgh = G.V[i].getInWeight(0);
                // edge_set.insert(std::make_tuple(in_node, i, wgh));
            }
            
        }
    }
    printf("Finish to find core graph edge with in/out flag\n");
    for(auto e = 0; e < G.n; e++){
        auto degreeV = G.V[e].getOutDegree();
        for(auto i = 0; i < degreeV; i++){
            if(edge_flag[e][i]){
              auto wgh = G.V[e].getOutWeight(i);
                edge_set.insert(std::make_tuple(e, G.V[e].getOutNeighbor(i), wgh));
            }
        }
        degreeV = G.V[e].getInDegree();
        for(auto i = 0; i < degreeV; i++){
            if(edge_flag_in[e][i]){
              auto wgh = G.V[e].getInWeight(i);
                edge_set.insert(std::make_tuple(G.V[e].getInNeighbor(i), e, wgh));
            }
        }
    }
    printf("Finish to add edges to edge sets\n");

    printf("Core Graph Edge Number is %ld\n", edge_set.size());
    free(query_result);
    ofstream of(prefix + ".core_edge_10s_sssp_p");
    for(auto edge : edge_set){
        of << get<0>(edge) << " " << get<1>(edge) << " " << get<2>(edge) << endl;
    }
    of.close();
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
    long n = GA.n;
    char* iFile = P.getArgument(0);
    std::string outFile = std::string(iFile);
    sampling_edge(GA, outFile);
}