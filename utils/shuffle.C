// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//Converts a SNAP graph (http://snap.stanford.edu/data/index.html) to
//Ligra adjacency graph format. To symmetrize the graph, pass the "-s"
//flag. For undirected graphs on SNAP, the "-s" flag must be passed
//since each edge appears in only one direction

#include "parseCommandLine.h"
#include "graphIO.h"
#include "parallel.h"
#include <stdlib.h>
#include "set"
#include <random>

using namespace std;

std::vector<uintT> generate_unique_random_numbers(uintT n, uintT N, uintT seed) {
    std::vector<uintT> numbers(N);
    std::iota(numbers.begin(), numbers.end(), 0); // Fill with 0, 1, ..., N-1

    std::default_random_engine engine(seed);
    std::shuffle(numbers.begin(), numbers.end(), engine);

    numbers.resize(n); // Keep only the first n numbers
    return numbers;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-n] number of batch [-z] batch size <input SNAP file>");
  char* iFile = P.getArgument(0);
  // char* oFile = P.getArgument(0);
  int batch_num = P.getOptionIntValue("-n", 0);
  double batch_size_ratio = P.getOptionDoubleValue("-z", 0);
  intE seed = P.getOptionIntValue("-r", 123456);

  edgeArray<uintT> G = readSNAP<uintT>(iFile);
  edge<intT> *E_tag = newA(edge<intT>,G.nonZeros);
  bool* dynamic = newA(bool,G.nonZeros);
  parallel_for (intT i = 0; i < G.nonZeros; i++) {
    E_tag[i].u = 0;
    E_tag[i].v = 0;
    dynamic[i] = false;
  }
  intE batch_size = G.nonZeros * batch_size_ratio;
  printf("without pruning num of nnz is %d\n",G.nonZeros);

  std::set<std::pair<uintT, uintT>> non_repeat_edges;
  for (auto i = 0; i < G.nonZeros; i++)
  {
    non_repeat_edges.insert(std::make_pair(G.E[i].u, G.E[i].v));
  }
  printf("with pruning num of edges is %ld\n", non_repeat_edges.size());
  printf("batch size is %d num of batch is %d, num of snapshot is %d\n", batch_size, batch_num, batch_num + 1);
  intE total_batch = 2 * batch_num * batch_size;
//   intE total_batch = batch_num * batch_size;

  auto random_selection = generate_unique_random_numbers(total_batch, G.nonZeros, seed);
  if (batch_num > 0 && batch_size > 0)
  {
    for (auto batch = 0; batch < batch_num; batch++)
    {
      std::string file_name = std::string(iFile) + ".add." + std::to_string(batch) + ".txt";
      std::ofstream of(file_name);
      for (auto i = 0; i < batch_size; i++)
      {
        of << G.E[random_selection[batch * batch_size + i]].u << " " << G.E[random_selection[batch * batch_size + i]].v << std::endl;
        E_tag[random_selection[batch * batch_size + i]].u = 0; // 0 means addition
        E_tag[random_selection[batch * batch_size + i]].v = batch;
        dynamic[random_selection[batch * batch_size + i]] = true;
      }
      of.close();
    }
    for (auto batch = batch_num; batch < 2 * batch_num; batch++)
    {
      std::string file_name = std::string(iFile) + ".del." + std::to_string(batch-batch_num) + ".txt";
      std::ofstream of(file_name);
      for (auto i = 0; i < batch_size; i++)
      {
        of << G.E[random_selection[batch * batch_size + i]].u << " " << G.E[random_selection[batch * batch_size + i]].v << std::endl;
        E_tag[random_selection[batch * batch_size + i]].u = 1; // 1 means deletion
        E_tag[random_selection[batch * batch_size + i]].v = batch - batch_num;
        dynamic[random_selection[batch * batch_size + i]] = true;        
      }
      of.close();
    }

    intE count = 0;
    int num_snapshots = batch_num + 1;
    for (int snapshot = 0; snapshot < num_snapshots; snapshot++)
    {
      std::string file_name = std::string(iFile) + "." +std::to_string(snapshot) + ".txt";
      std::ofstream of(file_name);
      for (auto i = 0; i < G.nonZeros; i++)
      {
        // not an dynamic edge or deletion id >= snapshot id or addition id  < snapshot id

        // S0 -> S1 -> S2 -> S3
        // S0 = common + del0 + del1 + del2
        // S1 = common + del1 + del2 + add0
        // S2 = common + del2 + add0 + add1
        // S3 = common + add0 + add1 + add2
        if ((!dynamic[i]) || (E_tag[i].u == 1 && E_tag[i].v >= snapshot) || (E_tag[i].u == 0 && E_tag[i].v < snapshot))
        {
          of << G.E[i].u << " " << G.E[i].v << std::endl;
          count++;
        }
      }
      of.close();
      fprintf(stderr, "snapshot %d, num of edges %d\n", snapshot, count);
      count = 0;
    }
  }
  else{
    printf("no need to be printed\n");
  }
  std::string file_name = std::string(iFile) + ".common";
    std::ofstream of(file_name);
    intE count = 0;
  for (auto i = 0; i < G.nonZeros; i++)
  {
    if (dynamic[i] == false){
        of << G.E[i].u << " " << G.E[i].v << std::endl;
        count++;
    }
  }
  fprintf(stderr, "intersection graph has num of edges %d\n", count);
  of.close();
  std::string file_name1 = std::string(iFile) + ".union";
    std::ofstream of1(file_name1);
  count = 0;
  for (auto i = 0; i < G.nonZeros; i++)
  {
      of1 << G.E[i].u << " " << G.E[i].v << std::endl;
      count++;
  }
  fprintf(stderr, "Union graph has num of edges %d\n", count);
  of1.close();  
}
