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
#include "set"
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-s] <G0> <G1>");
  char* iFile0 = P.getArgument(1);
  char* iFile1 = P.getArgument(0);
// Make sure all edges in G0 are in G1 and print all the edges in G0 that are in G1
  edgeArray<uintT> G = readSNAP<uintT>(iFile0);
  edgeArray<uintT> G1 = readSNAP<uintT>(iFile1);
  std::set<std::pair<uintT, uintT>> edges;
//   std::set<std::pair<uintT, uintT>> edges1;
  for (auto i = 0; i < G.nonZeros; i++)
  {
    edges.insert(std::make_pair(G.E[i].u, G.E[i].v));
  }
  for (auto i = 0; i < G1.nonZeros; i++)
  {
    edges.insert(std::make_pair(G1.E[i].u, G1.E[i].v));
  }
  printf("G0 has %d edges and G1 has %d edges\n", G.nonZeros, G1.nonZeros);
  intE found = 0;
  std::string outname = std::string(iFile0) + ".merge";
    ofstream of(outname);
    for (auto edge: edges)
    {
      of << edge.first << " " << edge.second << std::endl;
      found++;
    }
  printf("Found %d edges in interstcion of G1 and G2\n", found);
  of.close();
}
