/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2020 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#ifndef CK_GRIDWISE_GENERIC_2D_REDUCTION_MULTIBLOCK_ATOMIC_ADD_HPP
#define CK_GRIDWISE_GENERIC_2D_REDUCTION_MULTIBLOCK_ATOMIC_ADD_HPP

#include "reduction_common.hpp"
#include "reduction_operator.hpp"
#include "reduction_functions_binop.hpp"
#include "reduction_functions_blockwise.hpp"

#include "threadwise_tensor_slice_transfer.hpp"

namespace ck {

template <index_t BlockSize,
          typename srcDataType,
          typename dstDataType,
          typename compType,
          typename src2dDescType,
          typename dst1dDescType,
          ReduceTensorOp_t op,
          NanPropagation_t nanPropaOpt,
          index_t GredAccessesPerThreadInBlock>
struct GridwiseReduction_xy_to_x_multiblock_atomic_add
{
    static constexpr index_t ReduceDimVectorSize =
        math::gcd(GredAccessesPerThreadInBlock, CK_PARAM_REDUCE_DIM_VECTOR_SIZE);

    static constexpr index_t InvariantDimVectorSize = CK_PARAM_INVARIANT_DIM_VECTOR_SIZE;

    using opReduce       = typename reduce_binary_operator<compType, op>::opType;
    using preUnaryOpType = typename reduce_unary_operator<compType, op, true, true>::preUnaryOp;
    using posUnaryOpType = typename reduce_unary_operator<compType, op, true, true>::posUnaryOp;

    static constexpr auto buffer1dDesc =
        make_naive_tensor_descriptor_packed(make_tuple(Number<BlockSize>{}));

    using blockwise_reduce = BlockwiseReduction_1d_block_buffer<decltype(buffer1dDesc),
                                                                BlockSize,
                                                                opReduce,
                                                                nanPropaOpt>;

    static constexpr auto I0 = Number<0>{};

    static constexpr index_t BlockChunkSize = GredAccessesPerThreadInBlock * BlockSize;

    using binop = detail::binop_with_nan_check<nanPropaOpt, opReduce, compType>;

    __device__ static void Run(const src2dDescType& src2dDesc,
                               const dst1dDescType& dst1dDesc,
                               int origReduceLen,
                               int BlkGroupSize,
                               srcDataType alpha,
                               const srcDataType* const __restrict__ p_src_global,
                               dstDataType* const __restrict__ p_dst_global)
    {
        const auto zeroVal = opReduce::GetReductionZeroVal();

        // LDS
        __shared__ compType p_block_reduce_buffer[BlockSize];

        const auto src_global_buf = make_dynamic_buffer<AddressSpaceEnum_t::Global>(
            p_src_global, src2dDesc.GetElementSpaceSize(), type_convert<srcDataType>{}(zeroVal));
        auto dst_global_buf = make_dynamic_buffer<AddressSpaceEnum_t::Global>(
            p_dst_global, dst1dDesc.GetElementSpaceSize());

        auto block_reduce_buf =
            make_dynamic_buffer<AddressSpaceEnum_t::Lds>(p_block_reduce_buffer, BlockSize);

        StaticBuffer<AddressSpaceEnum_t::Vgpr,
                     compType,
                     InvariantDimVectorSize * GredAccessesPerThreadInBlock,
                     true>
            in_thread_buf;

        StaticBuffer<AddressSpaceEnum_t::Vgpr, compType, InvariantDimVectorSize, true>
            accuValue_buf;

        static_for<0, InvariantDimVectorSize, 1>{}([&](auto I) { accuValue_buf(I) = zeroVal; });

        const auto toReduceLength = src2dDesc.GetLength(Number<1>{});
        const int divider         = origReduceLen;

        const preUnaryOpType preUnaryOp(divider);
        const posUnaryOpType posUnaryOp(divider);

        const index_t thread_local_id = get_thread_local_1d_id();
        const index_t block_global_id = get_block_1d_id();
        const index_t blkgroup_id     = block_global_id / BlkGroupSize;
        const index_t block_local_id  = block_global_id % BlkGroupSize;

        const index_t reduceSizePerBlock =
            (((toReduceLength + BlkGroupSize - 1) / BlkGroupSize + BlockChunkSize - 1) /
             BlockChunkSize) *
            BlockChunkSize;

        using ThreadBufferLengths = Sequence<InvariantDimVectorSize, GredAccessesPerThreadInBlock>;
        constexpr auto ThreadBufferDesc = make_naive_tensor_descriptor_packed(
            make_tuple(Number<InvariantDimVectorSize>{}, Number<GredAccessesPerThreadInBlock>{}));

        auto threadwise_src_load = ThreadwiseTensorSliceTransfer_v2 < srcDataType, compType,
             src2dDescType, decltype(ThreadBufferDesc), ThreadBufferLengths, Sequence<0, 1>,
             (InvariantDimVectorSize > 1) ? 0 : 1,
             (InvariantDimVectorSize > 1) ? InvariantDimVectorSize : ReduceDimVectorSize, 1,
             false > (src2dDesc,
                      make_multi_index(blkgroup_id * InvariantDimVectorSize,
                                       block_local_id * reduceSizePerBlock +
                                           thread_local_id * GredAccessesPerThreadInBlock));

        constexpr auto in_thread_copy_step = make_multi_index(0, BlockChunkSize);

        const index_t toReduceChunks = reduceSizePerBlock / BlockChunkSize;

        for(index_t reducedChunks = 0; reducedChunks < toReduceChunks; reducedChunks++)
        {
            threadwise_src_load.Run(
                src2dDesc, src_global_buf, ThreadBufferDesc, make_tuple(I0, I0), in_thread_buf);

            static_for<0, InvariantDimVectorSize, 1>{}([&](auto I) {
                // do element-wise pre-reduction operation
                static_for<0, GredAccessesPerThreadInBlock, 1>{}([&](auto J) {
                    constexpr auto offset = I * Number<GredAccessesPerThreadInBlock>{} + J;
                    in_thread_buf(offset) = preUnaryOp(in_thread_buf[offset]);
                });

                // reduce on each thread-local slice
                static_for<0, GredAccessesPerThreadInBlock, 1>{}([&](auto J) {
                    constexpr auto offset = I * Number<GredAccessesPerThreadInBlock>{} + J;
                    binop::calculate(accuValue_buf(I), in_thread_buf[offset]);
                });
            });

            threadwise_src_load.MoveSrcSliceWindow(src2dDesc, in_thread_copy_step);
        }

        constexpr auto ReducedDataDesc =
            make_naive_tensor_descriptor_packed(make_tuple(Number<1>{}));

        // Each block executes a parallel reduction on the LDS, and by atomic-adding its reduced
        // output to the global location corresponding to each invariant dimension to get a
        // consistent reduced result for that invariant dimension. due to the using of vector_load,
        // each block/thread is involved into multiple invarirant dimensions.
        static_for<0, InvariantDimVectorSize, 1>{}([&](auto I) {
            block_reduce_buf(thread_local_id) = accuValue_buf[I];

            accuValue_buf(I) = zeroVal;

            __syncthreads();

            blockwise_reduce::Reduce(block_reduce_buf, accuValue_buf(I));

            if(thread_local_id == 0)
            {
                accuValue_buf(I) = posUnaryOp(accuValue_buf[I]);

                if(!float_equal_one{}(alpha))
                    accuValue_buf(I) *= type_convert<compType>{}(alpha);

                StaticBuffer<AddressSpaceEnum_t::Vgpr, dstDataType, 1, true> dstValue_buf;

                dstValue_buf(I0) = type_convert<dstDataType>{}(accuValue_buf[I]);

                auto threadwise_dst_store =
                    ThreadwiseTensorSliceTransfer_v1r3<dstDataType,
                                                       dstDataType,
                                                       decltype(ReducedDataDesc),
                                                       dst1dDescType,
                                                       Sequence<1>,
                                                       Sequence<0>,
                                                       0,
                                                       1,
                                                       InMemoryDataOperationEnum_t::AtomicAdd,
                                                       1,
                                                       true>(
                        dst1dDesc, make_multi_index(blkgroup_id * InvariantDimVectorSize + I()));

                threadwise_dst_store.Run(
                    ReducedDataDesc, make_tuple(I0), dstValue_buf, dst1dDesc, dst_global_buf);
            }
        });
    };
};

template <index_t BlockSize, typename dataType, typename global1dBufferDescType>
struct Gridwise_1d_global_buffer_set_value
{
    static constexpr auto I0 = Number<0>{};

    __device__ static void Run(const global1dBufferDescType& global1dBufferDesc,
                               dataType* const __restrict__ p_global,
                               dataType value)
    {
        const index_t thread_local_id = get_thread_local_1d_id();
        const index_t block_global_id = get_block_1d_id();

        const index_t thread_global_id = block_global_id * BlockSize + thread_local_id;

        StaticBuffer<AddressSpaceEnum_t::Vgpr, dataType, 1, true> value_buf;

        value_buf(I0) = value;

        constexpr auto valueBuffDesc = make_naive_tensor_descriptor_packed(make_tuple(Number<1>{}));

        auto global_buf = make_dynamic_buffer<AddressSpaceEnum_t::Global>(
            p_global, global1dBufferDesc.GetElementSpaceSize());

        if(thread_local_id < global1dBufferDesc.GetElementSize())
        {

            auto threadwise_store =
                ThreadwiseTensorSliceTransfer_v1r3<dataType,
                                                   dataType,
                                                   decltype(valueBuffDesc),
                                                   global1dBufferDescType,
                                                   Sequence<1>,
                                                   Sequence<0>,
                                                   0,
                                                   1,
                                                   InMemoryDataOperationEnum_t::Set,
                                                   1,
                                                   true>(global1dBufferDesc,
                                                         make_multi_index(thread_global_id));

            threadwise_store.Run(
                valueBuffDesc, make_tuple(I0), value_buf, global1dBufferDesc, global_buf);
        }
    };
};

} // namespace ck
#endif
