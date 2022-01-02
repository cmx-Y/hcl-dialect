// RUN: hcl-opt %s | hcl-opt | FileCheck %s

module {
    func @blur(%A: memref<10x10xf32>, %B: memref<10x8xf32>) -> memref<10x8xf32>
    {
        %li = hcl.create_loop_handle "i" : !hcl.LoopHandle
        %lj = hcl.create_loop_handle "j" : !hcl.LoopHandle
        %s = hcl.create_stage_handle "s" : !hcl.StageHandle
        affine.for %i = 0 to 10 {
            affine.for %j = 0 to 8 {
                %tmp = affine.load %A[%i, %j] : memref<10x10xf32>
                %tmp1 = affine.load %A[%i, %j+1] : memref<10x10xf32>
                %tmp2 = affine.load %A[%i, %j+2] : memref<10x10xf32>
                %sum = addf %tmp, %tmp1: f32
                %sum1 = addf %sum, %tmp2: f32
                affine.store %sum1, %B[%i, %j] : memref<10x8xf32>
            } { loop_name = "j" }
        } { loop_name = "i", stage_name = "s" }
        %buf = hcl.reuse_at(%s, %A: memref<10x10xf32>, 1) -> memref<3xf32>
        return %B : memref<10x8xf32>
    }
    func @blur5(%A: memref<10x10xf32>, %B: memref<10x5xf32>) -> memref<10x5xf32>
    {
        %li = hcl.create_loop_handle "i" : !hcl.LoopHandle
        %lj = hcl.create_loop_handle "j" : !hcl.LoopHandle
        %s = hcl.create_stage_handle "s" : !hcl.StageHandle
        affine.for %i = 0 to 10 {
            affine.for %j = 0 to 5 {
                %tmp = affine.load %A[%i, %j] : memref<10x10xf32>
                %tmp1 = affine.load %A[%i, %j+1] : memref<10x10xf32>
                %tmp2 = affine.load %A[%i, %j+2] : memref<10x10xf32>
                %tmp3 = affine.load %A[%i, %j+3] : memref<10x10xf32>
                %tmp4 = affine.load %A[%i, %j+4] : memref<10x10xf32>
                %sum = addf %tmp, %tmp1: f32
                %sum1 = addf %tmp2, %tmp3: f32
                %sum2 = addf %sum1, %tmp4: f32
                %sum3 = addf %sum, %sum2: f32
                affine.store %sum3, %B[%i, %j] : memref<10x5xf32>
            } { loop_name = "j" }
        } { loop_name = "i", stage_name = "s" }
        %buf = hcl.reuse_at(%s, %A: memref<10x10xf32>, 1) -> memref<5xf32>
        return %B : memref<10x5xf32>
    }
    func @blur_x(%A: memref<10x10xf32>, %B: memref<8x10xf32>) -> memref<8x10xf32>
    {
        %li = hcl.create_loop_handle "i" : !hcl.LoopHandle
        %lj = hcl.create_loop_handle "j" : !hcl.LoopHandle
        %s = hcl.create_stage_handle "s" : !hcl.StageHandle
        affine.for %i = 0 to 8 {
            affine.for %j = 0 to 10 {
                %tmp = affine.load %A[%i, %j] : memref<10x10xf32>
                %tmp1 = affine.load %A[%i+1, %j] : memref<10x10xf32>
                %tmp2 = affine.load %A[%i+2, %j] : memref<10x10xf32>
                %sum = addf %tmp, %tmp1: f32
                %sum1 = addf %sum, %tmp2: f32
                affine.store %sum1, %B[%i, %j] : memref<8x10xf32>
            } { loop_name = "j" }
        } { loop_name = "i", stage_name = "s" }
        %buf = hcl.reuse_at(%s, %A: memref<10x10xf32>, 0) -> memref<3x10xf32>
        return %B : memref<8x10xf32>
    }
    // func @blur_reduction(%A: memref<10x10xf32>, %B: memref<10x8xf32>) -> memref<10x8xf32>
    // {
    //     %li = hcl.create_loop_handle "i" : !hcl.LoopHandle
    //     %lj = hcl.create_loop_handle "j" : !hcl.LoopHandle
    //     %s = hcl.create_stage_handle "s" : !hcl.StageHandle
    //     affine.for %i = 0 to 10 {
    //         affine.for %j = 0 to 8 {
    //             %zero = constant 0.0 : f32
    //             %sum = affine.for %r = 0 to 3 iter_args(%sum_iter = %zero) -> (f32) {
    //                 %tmp = affine.load %A[%i, %j+%r] : memref<10x10xf32>
    //                 %sum_next = addf %sum_iter, %tmp: f32
    //                 affine.yield %sum_next : f32
    //             } { loop_name = "r", reduction = 1}
    //             affine.store %sum, %B[%i, %j] : memref<10x8xf32>
    //         } { loop_name = "j" }
    //     } { loop_name = "i", stage_name = "s" }
    //     %buf = hcl.reuse_at(%s, %A: memref<10x10xf32>, 1) -> memref<3xf32>
    //     return %B : memref<10x8xf32>
    // }
    func @conv2d(%A: memref<10x10xf32>, %B: memref<8x8xf32>) -> memref<8x8xf32>
    {
        %li = hcl.create_loop_handle "i" : !hcl.LoopHandle
        %lj = hcl.create_loop_handle "j" : !hcl.LoopHandle
        %s = hcl.create_stage_handle "s" : !hcl.StageHandle
        affine.for %i = 0 to 8 {
            affine.for %j = 0 to 8 {
                %tmp = affine.load %A[%i, %j] : memref<10x10xf32>
                %tmp1 = affine.load %A[%i, %j+1] : memref<10x10xf32>
                %tmp2 = affine.load %A[%i, %j+2] : memref<10x10xf32>
                %tmp3 = affine.load %A[%i+1, %j] : memref<10x10xf32>
                %tmp4 = affine.load %A[%i+1, %j+1] : memref<10x10xf32>
                %tmp5 = affine.load %A[%i+1, %j+2] : memref<10x10xf32>
                %tmp6 = affine.load %A[%i+2, %j] : memref<10x10xf32>
                %tmp7 = affine.load %A[%i+2, %j+1] : memref<10x10xf32>
                %tmp8 = affine.load %A[%i+2, %j+2] : memref<10x10xf32>
                %sum = addf %tmp, %tmp1: f32
                %sum1 = addf %tmp2, %tmp3: f32
                %sum2 = addf %sum1, %tmp4: f32
                %sum3 = addf %sum, %sum2: f32
                affine.store %sum3, %B[%i, %j] : memref<8x8xf32>
            } { loop_name = "j" }
        } { loop_name = "i", stage_name = "s" }
        %buf = hcl.reuse_at(%s, %A: memref<10x10xf32>, 0) -> memref<3x10xf32>
        hcl.partition(%buf: memref<3x10xf32>, "CompletePartition", 1)
        return %B : memref<8x8xf32>
    }
}