//===----------------------------------------------------------------------===//
//
// Copyright 2020-2021 The ScaleHLS Authors.
//
//===----------------------------------------------------------------------===//

#ifndef SCALEHLS_DIALECT_HLSCPP_VISITOR_H
#define SCALEHLS_DIALECT_HLSCPP_VISITOR_H

#include "hcl/Dialect/HeteroCLDialect.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/SCF.h"
#include "llvm/ADT/TypeSwitch.h"

namespace mlir {
namespace hcl {

/// This class is a visitor for SSACFG operation nodes.
template <typename ConcreteType, typename ResultType, typename... ExtraArgs>
class HLSCppVisitorBase {
public:
  ResultType dispatchVisitor(Operation *op, ExtraArgs... args) {
    auto *thisCast = static_cast<ConcreteType *>(this);
    return TypeSwitch<Operation *, ResultType>(op)
        .template Case<
            // SCF statements.
            scf::ForOp, scf::IfOp, scf::ParallelOp, scf::ReduceOp,
            scf::ReduceReturnOp, scf::YieldOp,
            // Affine statements.
            AffineForOp, AffineIfOp, AffineParallelOp, AffineApplyOp,
            AffineMaxOp, AffineMinOp, AffineLoadOp, AffineStoreOp,
            AffineYieldOp, AffineVectorLoadOp, AffineVectorStoreOp,
            AffineDmaStartOp, AffineDmaWaitOp,
            // Memref-related statements.
            memref::AllocOp, memref::AllocaOp, memref::LoadOp, memref::StoreOp,
            memref::DeallocOp, memref::DmaStartOp, memref::DmaWaitOp,
            memref::ViewOp, memref::SubViewOp, AtomicRMWOp, GenericAtomicRMWOp,
            AtomicYieldOp,
            // Tensor-related statements.
            memref::TensorLoadOp, memref::TensorStoreOp, memref::BufferCastOp,
            SplatOp, memref::DimOp, RankOp,
            // Unary expressions.
            // math::AbsOp, math::CeilOp, math::CosOp, math::SinOp, math::TanhOp,
            // math::SqrtOp, math::RsqrtOp, math::ExpOp, math::Exp2Op, math::LogOp,
            // math::Log2Op, math::Log10Op, NegFOp,
            // Float binary expressions.
            CmpFOp, AddFOp, SubFOp, MulFOp,
            DivFOp, RemFOp,
            // // Integer binary expressions.
            // CmpIOp, AddIOp, SubIOp, MulIOp,
            // DivSIOp, RemSIOp, DivUIOp, RemUIOp,
            // XOrIOp, AndIOp, OrIOp, ShLIOp,
            // ShRSIOp, ShRUIOp,
            // // Special operations.
            ReturnOp
            // CallOp, ReturnOp, SelectOp, ConstantOp, ConstantOp,
            // TruncIOp, TruncFOp, ExtUIOp, ExtSIOp,
            // IndexCastOp, UIToFPOp, SIToFPOp,
            // FPToSIOp, FPToUIOp,
            // // HLSCpp operations.
            // AssignOp, CastOp, MulOp, AddOp
            >([&](auto opNode) -> ResultType {
          return thisCast->visitOp(opNode, args...);
        })
        .Default([&](auto opNode) -> ResultType {
          return thisCast->visitInvalidOp(op, args...);
        });
  }

  /// This callback is invoked on any invalid operations.
  ResultType visitInvalidOp(Operation *op, ExtraArgs... args) {
    op->emitOpError("is unsupported operation.");
    abort();
  }

  /// This callback is invoked on any operations that are not handled by the
  /// concrete visitor.
  ResultType visitUnhandledOp(Operation *op, ExtraArgs... args) {
    return ResultType();
  }

#define HANDLE(OPTYPE)                                                         \
  ResultType visitOp(OPTYPE op, ExtraArgs... args) {                           \
    return static_cast<ConcreteType *>(this)->visitUnhandledOp(op, args...);   \
  }

  // SCF statements.
  HANDLE(scf::ForOp);
  HANDLE(scf::IfOp);
  HANDLE(scf::ParallelOp);
  HANDLE(scf::ReduceOp);
  HANDLE(scf::ReduceReturnOp);
  HANDLE(scf::YieldOp);

  // Affine statements.
  HANDLE(AffineForOp);
  HANDLE(AffineIfOp);
  HANDLE(AffineParallelOp);
  HANDLE(AffineApplyOp);
  HANDLE(AffineMaxOp);
  HANDLE(AffineMinOp);
  HANDLE(AffineLoadOp);
  HANDLE(AffineStoreOp);
  HANDLE(AffineYieldOp);
  HANDLE(AffineVectorLoadOp);
  HANDLE(AffineVectorStoreOp);
  HANDLE(AffineDmaStartOp);
  HANDLE(AffineDmaWaitOp);

  // Memref-related statements.
  HANDLE(memref::AllocOp);
  HANDLE(memref::AllocaOp);
  HANDLE(memref::LoadOp);
  HANDLE(memref::StoreOp);
  HANDLE(memref::DeallocOp);
  HANDLE(memref::DmaStartOp);
  HANDLE(memref::DmaWaitOp);
  HANDLE(AtomicRMWOp);
  HANDLE(GenericAtomicRMWOp);
  HANDLE(AtomicYieldOp);
  HANDLE(memref::ViewOp);
  HANDLE(memref::SubViewOp);

  // Tensor-related statements.
  HANDLE(memref::TensorLoadOp);
  HANDLE(memref::TensorStoreOp);
  HANDLE(memref::BufferCastOp);
  HANDLE(SplatOp);
  HANDLE(memref::DimOp);
  HANDLE(RankOp);

  // Unary expressions.
  // HANDLE(math::AbsOp);
  // HANDLE(math::CeilOp);
  // HANDLE(math::CosOp);
  // HANDLE(math::SinOp);
  // HANDLE(math::TanhOp);
  // HANDLE(math::SqrtOp);
  // HANDLE(math::RsqrtOp);
  // HANDLE(math::ExpOp);
  // HANDLE(math::Exp2Op);
  // HANDLE(math::LogOp);
  // HANDLE(math::Log2Op);
  // HANDLE(math::Log10Op);
  // HANDLE(NegFOp);

  // Float binary expressions.
  HANDLE(CmpFOp);
  HANDLE(AddFOp);
  HANDLE(SubFOp);
  HANDLE(MulFOp);
  HANDLE(DivFOp);
  HANDLE(RemFOp);

  // // Integer binary expressions.
  // HANDLE(CmpIOp);
  // HANDLE(AddIOp);
  // HANDLE(SubIOp);
  // HANDLE(MulIOp);
  // HANDLE(DivSIOp);
  // HANDLE(RemSIOp);
  // HANDLE(DivUIOp);
  // HANDLE(RemUIOp);
  // HANDLE(XOrIOp);
  // HANDLE(AndIOp);
  // HANDLE(OrIOp);
  // HANDLE(ShLIOp);
  // HANDLE(ShRSIOp);
  // HANDLE(ShRUIOp);

  // // Special operations.
  // HANDLE(CallOp);
  HANDLE(ReturnOp);
  // HANDLE(SelectOp);
  // HANDLE(ConstantOp);
  // HANDLE(ConstantOp);
  // HANDLE(TruncIOp);
  // HANDLE(TruncFOp);
  // HANDLE(ExtUIOp);
  // HANDLE(ExtSIOp);
  // HANDLE(ExtFOp);
  // HANDLE(IndexCastOp);
  // HANDLE(UIToFPOp);
  // HANDLE(SIToFPOp);
  // HANDLE(FPToUIOp);
  // HANDLE(FPToSIOp);

  // // HLSCpp operations.
  // HANDLE(AssignOp);
  // HANDLE(CastOp);
  // HANDLE(AddOp);
  // HANDLE(MulOp);
#undef HANDLE
};
} // namespace hcl
} // namespace mlir

#endif // SCALEHLS_DIALECT_HLSCPP_VISITOR_H