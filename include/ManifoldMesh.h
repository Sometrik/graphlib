#ifndef _MANIFOLDMESH_H_
#define _MANIFOLDMESH_H_

#include <Graph.h>

// MBFV		Make Body Face Vertex (initial op)	KBFV
// MEV		Make an edge and a vertex		KEV
// MEKL		Make an edge and kill a loop		KEKL
// MEF		Make a face and an edge 		KEF
// MEKBFL	Make edge kill body face loop		KEMBFL
// MFKLG	Make face kill loop genus		KFMLG
// KFEVMG
// KFEVM
// MME
// ESPLIT
// KVE

// MSFV 	Make a shell, a face and a vertex 	+1 	
// MSG 	Make a shell and a hole 	
// MEKL 	

class ManifoldMesh : public Graph {
 public:
  ManifoldMesh(int _id = 0);
  
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new ManifoldMesh(*this); }

  bool checkConsistency() const override;
};

#endif
