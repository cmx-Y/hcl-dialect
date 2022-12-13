//===----------------------------------------------------------------------===//
//
// Copyright 2021-2022 The HCL-MLIR Authors.
//
//===----------------------------------------------------------------------===//

#include "PassDetail.h"

#include "hcl/Dialect/HeteroCLDialect.h"
#include "hcl/Dialect/HeteroCLOps.h"
#include "hcl/Dialect/HeteroCLTypes.h"
#include "hcl/Transforms/Passes.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"

#include <map>
#include <set>

using namespace mlir;
using namespace hcl;

namespace mlir {
namespace hcl {

class Node {
  // Member variables
  Operation *op;
  DeviceEnum device;
  std::vector<Node *> upstream;
  std::vector<Node *> downstream;
  std::vector<Operation *> consumedMemRefs;
  std::vector<Operation *> producedMemRefs;

public:
  Node(Operation *op) : op(op) {}
  void addUpstream(Node *node) { upstream.push_back(node); }
  void addDownstream(Node *node) { downstream.push_back(node); }
  void addConsumedMemRef(Operation *memRef) { consumedMemRefs.push_back(memRef); }
  void addProducedMemRef(Operation *memRef) { producedMemRefs.push_back(memRef); }
  std::vector<Operation *> getConsumedMemRefs() { return consumedMemRefs; }
  void print() {
    llvm::outs() << "Node: " << this->getName() << "\n";
    llvm::outs() << "Upstream: ";
    for (auto node : this->upstream) {
      llvm::outs() << node->getName() << " ";
    }
    llvm::outs() << "\n";
    llvm::outs() << "Downstream: ";
    for (auto node : this->downstream) {
      llvm::outs() << node->getName() << " ";
    }
    llvm::outs() << "\n";
  }
  std::string getName() {
    // check if "op_name" attribute exists
    if (this->op->getAttr("op_name")) {
      return this->op->getAttr("op_name").cast<StringAttr>().getValue().str();
    } else if (this->op->getAttr("loop_name")) {
      return this->op->getAttr("loop_name").cast<StringAttr>().getValue().str();
    } else {
      return this->op->getName().getStringRef().str();
    }
  }
};

class DataFlowGraph {
  // Member variables
  std::map<std::string, Node *> nodeMap;

public:
  void addNode(Node *node) { 
    this->nodeMap[node->getName()] = node;
  }
  void addEdge(Node *src, Node *dst) {
    src->addDownstream(dst);
    dst->addUpstream(src);
  }
  Node* getNode(std::string name) {
    return this->nodeMap[name];
  }
  void getNodeByConsumedMemRefs(Operation *memRef, std::vector<Node *> &consumerNodes) {
    for (auto node : this->nodeMap) {
      for (auto consumedMemRef : node.second->getConsumedMemRefs()) {
        if (consumedMemRef == memRef) {
          consumerNodes.push_back(node.second);
        }
      }
    }
  }
  void print() {
    // print the graph
    for (auto node : this->nodeMap) {
      node.second->print();
    }
  }
};

void getAllLoadedMemRefs(Operation *op, std::set<Operation *> &memRefs) {
  SmallVector<Operation *, 8> loadOps;
  op->walk([&](Operation *op) {
    if (isa<AffineLoadOp>(op)) {
      loadOps.push_back(op);
    } else if (isa<memref::LoadOp>(op)) {
      loadOps.push_back(op);
    }
  });

  // add memrefs to the set
  for (auto loadOp : loadOps) {
    auto operand = loadOp->getOperand(0);
    // check if operand defining op is a block arg
    if (operand.isa<BlockArgument>()) {
      // get block arg index
      unsigned int index = operand.cast<BlockArgument>().getArgNumber();
      memRefs.insert(reinterpret_cast<Operation *>(index));
    } else {
      memRefs.insert(loadOp->getOperand(0).getDefiningOp());
    }
  }
}

void getAllStoredMemRefs(Operation *op, std::set<Operation *> &memRefs) {
  SmallVector<Operation *, 8> storeOps;
  op->walk([&](Operation *op) {
    if (isa<AffineStoreOp>(op)) {
      storeOps.push_back(op);
    } else if (isa<memref::StoreOp>(op)) {
      storeOps.push_back(op);
    }
  });

  // add memrefs to the set
  for (auto storeOp : storeOps) {
    auto operand = storeOp->getOperand(1);
    if (operand.isa<BlockArgument>()) {
      // get block arg index
      unsigned int index = operand.cast<BlockArgument>().getArgNumber();
      memRefs.insert(reinterpret_cast<Operation *>(index));
    } else {
      memRefs.insert(storeOp->getOperand(1).getDefiningOp());
    }
  }
}

DataFlowGraph buildDFGInScope(Operation &scope_op) {
  // build a data flow graph
  // given an operation as the scope of the graph
  DataFlowGraph graph;
  std::map<Operation *, Node *> latestProducer;
  for (auto &region : scope_op.getRegions()) {
    for (auto &block : region.getBlocks()) {
      for (auto &op : block.getOperations()) {
        // skip op that is not a loop
        if (!isa<AffineForOp>(op) && !isa<scf::ForOp>(op)) {
          continue;
        }
        // create a node for each op
        Node *node = new Node(&op);
        graph.addNode(node);
        // get all the memrefs consumed and produced by the op
        std::set<Operation *> consumedMemRefs;
        std::set<Operation *> producedMemRefs;
        getAllLoadedMemRefs(&op, consumedMemRefs);
        getAllStoredMemRefs(&op, producedMemRefs);
        // add edges to the graph
        for (auto memRef : consumedMemRefs) {
          // get the node that produces the memref
          // add an edge from the node to the current node
          // check if memRef is in latestProducer map
          node->addConsumedMemRef(memRef);
          if (latestProducer.find(memRef) != latestProducer.end()) {
            Node *producer = latestProducer[memRef];
            graph.addEdge(producer, node);
          }
        }
        // update the latest producer for each memref
        for (auto memRef : producedMemRefs) {
          node->addProducedMemRef(memRef);
          latestProducer[memRef] = node;
        }
      }
    }
  }
  return graph;
}

void buildHierarchicalDFG(ModuleOp &module, std::map<Operation *, DataFlowGraph> &hierarchicalDFG) {
  // build a hierarchical data flow graph
  // given a module
  // get all the top level ops
  SmallVector<Operation *, 4> topLevelOps;
  module.walk([&](Operation *op) {
    if (isa<AffineForOp>(op) || isa<scf::ForOp>(op) || isa<func::FuncOp>(op)) {
      topLevelOps.push_back(op);
    }
  });
  // build a data flow graph for each top level op
  for (auto op : topLevelOps) {
    DataFlowGraph graph = buildDFGInScope(*op);
    hierarchicalDFG[op] = graph;
  }
}


/// Pass entry point
bool applyDataPlacement(ModuleOp &module) {

  // get all HostXcelTo ops
  SmallVector<Operation *, 4> hostXcelToOps;
  module.walk([&](Operation *op) {
    if (isa<HostXcelToOp>(op)) {
      hostXcelToOps.push_back(op);
    }
  });


  for (auto op : hostXcelToOps) {
    HostXcelToOp toOp = dyn_cast<HostXcelToOp>(op);
    auto target = toOp.target();
    auto device = toOp.device();
    auto optional_axis = toOp.axis();
    llvm::outs() << "target: " << target << "\n";
    // llvm::outs() << "device: " << device << "\n";
    // llvm::outs() << "axis: " << optional_axis << "\n";
    // check if axis has value
    if (optional_axis) {
      llvm::outs() << "axis has value: " << optional_axis << "\n";
    } else {
      llvm::outs() << "axis has no value\n";
    }
  }

  // for (auto func : module.getOps<func::FuncOp>()) {
  //   DataFlowGraph graph = buildDFG(*func.getOperation());
  //   graph.print();
  // }

  // try creating a new module
  // this worked:
  // OpBuilder builder(module.getContext());
  // builder.setInsertionPointToStart(module.getBody());
  // builder.create<ModuleOp>(module.getLoc());

  // move all ops in the old module to the new module
  //   newModule.getBody()->getOperations().splice(
  //         newModule.getBody()->begin(), module.getBody()->getOperations());
  return true;
}

} // namespace hcl
} // namespace mlir

namespace {
struct HCLDataPlacementTransformation
    : public DataPlacementBase<HCLDataPlacementTransformation> {
  void runOnOperation() override {
    auto mod = getOperation();
    if (!applyDataPlacement(mod)) {
      signalPassFailure();
    }
  }
};
} // namespace

namespace mlir {
namespace hcl {
std::unique_ptr<OperationPass<ModuleOp>> createDataPlacementPass() {
  return std::make_unique<HCLDataPlacementTransformation>();
}
} // namespace hcl
} // namespace mlir