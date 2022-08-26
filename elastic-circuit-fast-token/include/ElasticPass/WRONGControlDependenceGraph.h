/*
 * Author: Ayatallah Elakhras
*/
//----------------------------------------------------------------------------//
//
// This file defines the ControlDependenceGraph class as an LLVM pass, which allows fast and 
// efficient control dependence queries. It is based on Ferrante et al.'s "The 
// Program Dependence Graph and Its Use in Optimization"
//
//----------------------------------------------------------------------------//

#pragma once

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/DOTGraphTraits.h"

#include <map>
#include <set>
#include <iterator>

namespace llvm {

class BasicBlock;
class ControlDependenceGraphBase;

class ControlDependenceNode {
public:
  enum EdgeType { TRUE, FALSE, OTHER };
  typedef std::set<ControlDependenceNode *>::iterator       node_iterator;
  typedef std::set<ControlDependenceNode *>::const_iterator const_node_iterator;

  struct edge_iterator {
    typedef node_iterator::value_type      value_type;
    typedef node_iterator::difference_type difference_type;
    typedef node_iterator::reference       reference;
    typedef node_iterator::pointer         pointer;
    typedef std::input_iterator_tag        iterator_category;

    edge_iterator(ControlDependenceNode *n) : 
      node(n), stage(TRUE), it(n->TrueChildren.begin()), end(n->TrueChildren.end()) {
      while ((stage != OTHER) && (it == end)) this->operator++();
    }
    edge_iterator(ControlDependenceNode *n, EdgeType t, node_iterator i, node_iterator e) :
      node(n), stage(t), it(i), end(e) {
      while ((stage != OTHER) && (it == end)) this->operator++();
    }
    EdgeType type() const { return stage; }
    bool operator==(edge_iterator const &other) const { 
      return (this->stage == other.stage) && (this->it == other.it);
    }
    bool operator!=(edge_iterator const &other) const { return !(*this == other); }
    reference operator*()  { return *this->it; }
    pointer   operator->() { return &*this->it; }
    edge_iterator& operator++() {
      if (it != end) ++it;
      while ((stage != OTHER) && (it == end)) {
	if (stage == TRUE) {
	  it = node->FalseChildren.begin();
	  end = node->FalseChildren.end();
	  stage = FALSE;
	} else {
	  it = node->OtherChildren.begin();
	  end = node->OtherChildren.end();
	  stage = OTHER;
	}
      }
      return *this;
    }
    edge_iterator operator++(int) {
      edge_iterator ret(*this);
      assert(ret.stage == OTHER || ret.it != ret.end);
      this->operator++();
      return ret;
    }
  private:
    ControlDependenceNode *node;
    EdgeType stage;
    node_iterator it, end;
  };

  edge_iterator begin() { return edge_iterator(this); }
  edge_iterator end()   { return edge_iterator(this, OTHER, OtherChildren.end(), OtherChildren.end()); }

  node_iterator true_begin()   { return TrueChildren.begin(); }
  node_iterator true_end()     { return TrueChildren.end(); }

  node_iterator false_begin()  { return FalseChildren.begin(); }
  node_iterator false_end()    { return FalseChildren.end(); }

  node_iterator other_begin()  { return OtherChildren.begin(); }
  node_iterator other_end()    { return OtherChildren.end(); }

  node_iterator parent_begin() { return Parents.begin(); }
  node_iterator parent_end()   { return Parents.end(); }
  const_node_iterator parent_begin() const { return Parents.begin(); }
  const_node_iterator parent_end()   const { return Parents.end(); }

  BasicBlock *getBlock() const { return TheBB; }
  size_t getNumParents() const { return Parents.size(); }
  size_t getNumChildren() const { 
    return TrueChildren.size() + FalseChildren.size() + OtherChildren.size();
  }
  bool isRegion() const { return TheBB == NULL; }
  const ControlDependenceNode *enclosingRegion() const;

private:
  BasicBlock *TheBB;
  std::set<ControlDependenceNode *> Parents;
  std::set<ControlDependenceNode *> TrueChildren;
  std::set<ControlDependenceNode *> FalseChildren;
  std::set<ControlDependenceNode *> OtherChildren;

  friend class ControlDependenceGraphBase;

  void clearAllChildren() {
    TrueChildren.clear();
    FalseChildren.clear();
    OtherChildren.clear();
  }
  void clearAllParents() { Parents.clear(); }

  void addTrue(ControlDependenceNode *Child);
  void addFalse(ControlDependenceNode *Child);
  void addOther(ControlDependenceNode *Child);
  void addParent(ControlDependenceNode *Parent);
  void removeTrue(ControlDependenceNode *Child);
  void removeFalse(ControlDependenceNode *Child);
  void removeOther(ControlDependenceNode *Child);
  void removeParent(ControlDependenceNode *Child);

  ControlDependenceNode() : TheBB(NULL) {}
  ControlDependenceNode(BasicBlock *bb) : TheBB(bb) {}
};

template <> struct GraphTraits<ControlDependenceNode *> {
	typedef ControlDependenceNode NodeType;
	typedef NodeType::edge_iterator ChildIteratorType;

	// Aya: added the following line since the default types of the df_iterator class require the defintion of "NodeRef" type
	typedef ControlDependenceNode NodeRef;

	//static NodeType *getEntryNode(NodeType *N) { return N; }
	static NodeRef *getEntryNode(NodeRef *N) { return N; }

	static inline ChildIteratorType child_begin(NodeType *N) {
		return N->begin();
	}
	static inline ChildIteratorType child_end(NodeType *N) {
		return N->end();
	}

	typedef df_iterator<ControlDependenceNode*> nodes_iterator;

	/*static nodes_iterator nodes_begin(ControlDependenceNode *N) {
		return df_begin(getEntryNode(N));
	}
	static nodes_iterator nodes_end(ControlDependenceNode *N) {
		return df_end(getEntryNode(N));
	}*/
};
  
class ControlDependenceGraphBase {
public:
  ControlDependenceGraphBase() : root(NULL) {}
  virtual ~ControlDependenceGraphBase() { releaseMemory(); }
  virtual void releaseMemory() {
    for (ControlDependenceNode::node_iterator n = nodes.begin(), e = nodes.end();
	 n != e; ++n) delete *n;
    nodes.clear();
    bbMap.clear();
    root = NULL;
  }

  void graphForFunction(Function &F, PostDominatorTreeWrapperPass &pdt);

  ControlDependenceNode *getRoot()             { return root; }
  const ControlDependenceNode *getRoot() const { return root; }
  ControlDependenceNode *operator[](const BasicBlock *BB)             { return getNode(BB); }
  const ControlDependenceNode *operator[](const BasicBlock *BB) const { return getNode(BB); }
  ControlDependenceNode *getNode(const BasicBlock *BB) { 
    return bbMap[BB];
  }
  const ControlDependenceNode *getNode(const BasicBlock *BB) const {
    return (bbMap.find(BB) != bbMap.end()) ? bbMap.find(BB)->second : NULL;
  }
  bool controls(BasicBlock *A, BasicBlock *B) const;
  bool influences(BasicBlock *A, BasicBlock *B) const;
  const ControlDependenceNode *enclosingRegion(BasicBlock *BB) const;

private:
  ControlDependenceNode *root;
  std::set<ControlDependenceNode *> nodes;
  std::map<const BasicBlock *,ControlDependenceNode *> bbMap;
  static ControlDependenceNode::EdgeType getEdgeType(const BasicBlock *, const BasicBlock *);
  void computeDependencies(Function &F, PostDominatorTreeWrapperPass &pdt);
  void insertRegions(PostDominatorTreeWrapperPass &pdt);
};

// our LLVM pass that computes the CDG
class ControlDependenceGraph : public FunctionPass, public ControlDependenceGraphBase {
// Aya: replaced all PostDominatorTree with PostDominatorTreeWrapperPass since PostDominatorTree is no longer a pass in LLVM
public:
  static char ID;

  ControlDependenceGraph(): FunctionPass(ID), ControlDependenceGraphBase() {}
  virtual ~ControlDependenceGraph() { }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.setPreservesAll();
  }
  virtual bool runOnFunction(Function &F) {
    PostDominatorTreeWrapperPass &pdt = getAnalysis<PostDominatorTreeWrapperPass>();
    graphForFunction(F,pdt);
    return false;
  }
};


template <> struct GraphTraits<ControlDependenceGraph *>
  : public GraphTraits<ControlDependenceNode *> {
  static NodeType *getEntryNode(ControlDependenceGraph *CD) {
    return CD->getRoot();
  }

  /*static nodes_iterator nodes_begin(ControlDependenceGraph *CD) {
    if (getEntryNode(CD))
      return df_begin(getEntryNode(CD));
    else
      return df_end(getEntryNode(CD));
  }

  static nodes_iterator nodes_end(ControlDependenceGraph *CD) {
    return df_end(getEntryNode(CD));
  }*/
};

template <> struct DOTGraphTraits<ControlDependenceGraph *>
  : public DefaultDOTGraphTraits {
  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}

  static std::string getGraphName(ControlDependenceGraph *Graph) {
    return "Control dependence graph";
  }

  std::string getNodeLabel(ControlDependenceNode *Node, ControlDependenceGraph *Graph) {
    if (Node->isRegion()) {
      return "REGION";
    } else {
      return Node->getBlock()->hasName() ? Node->getBlock()->getName() : "ENTRY";
    }
  }

  static std::string getEdgeSourceLabel(ControlDependenceNode *Node, ControlDependenceNode::edge_iterator I) {
    switch (I.type()) {
    case ControlDependenceNode::TRUE:
      return "T";
    case ControlDependenceNode::FALSE:
      return "F";
    case ControlDependenceNode::OTHER:
      return "";
    }
  }
};

/*class ControlDependenceGraphs : public ModulePass {
public:
  static char ID;

  ControlDependenceGraphs() : ModulePass(ID) {}
  virtual ~ControlDependenceGraphs() {
    graphs.clear();
  }

  virtual bool runOnModule(Module &M) {
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration())
	continue;
      ControlDependenceGraphBase &cdg = graphs[F];
      PostDominatorTree &pdt = getAnalysis<PostDominatorTree>(*F);
      cdg.graphForFunction(*F,pdt);
    }
    return false;
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<PostDominatorTree>();
    AU.setPreservesAll();
  }

  ControlDependenceGraphBase &operator[](const Function *F) { return graphs[F]; }
  ControlDependenceGraphBase &graphFor(const Function *F) { return graphs[F]; }
private:
  std::map<const Function *, ControlDependenceGraphBase> graphs;
};*/

} // namespace llvm
