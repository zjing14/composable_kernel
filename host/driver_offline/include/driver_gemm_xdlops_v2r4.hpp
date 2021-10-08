#ifndef DRIVER_GEMM_XDLOPS_V2R4
#define DRIVER_GEMM_XDLOPS_V2R4

#include "common_header.hpp"
#include "tensor_descriptor.hpp"
#include "tensor_descriptor_helper.hpp"
#include "gridwise_gemm_xdlops_v2r4.hpp"

template <ck::index_t BlockSize,
          typename FloatAB,
          typename FloatAcc,
          typename FloatC,
          ck::InMemoryDataOperationEnum_t CGlobalMemoryDataOperation,
          typename ABK0MK1GridDesc,
          typename BBK0NK1GridDesc,
          typename CMNGridDesc,
          ck::index_t MPerBlock,
          ck::index_t NPerBlock,
          ck::index_t KPerBlock,
          ck::index_t MPerXDL,
          ck::index_t NPerXDL,
          ck::index_t K1,
          ck::index_t MRepeat,
          ck::index_t NRepeat,
          typename ABlockTransferThreadSliceLengths_K0_M_K1,
          typename ABlockTransferThreadClusterLengths_K0_M_K1,
          typename ABlockTransferThreadClusterArrangeOrder,
          typename ABlockTransferSrcAccessOrder,
          ck::index_t ABlockTransferSrcVectorDim,
          ck::index_t ABlockTransferSrcScalarPerVector,
          ck::index_t ABlockTransferDstScalarPerVector_K1,
          bool AThreadTransferSrcResetCoordinateAfterRun,
          typename BBlockTransferThreadSliceLengths_K0_N_K1,
          typename BBlockTransferThreadClusterLengths_K0_N_K1,
          typename BBlockTransferThreadClusterArrangeOrder,
          typename BBlockTransferSrcAccessOrder,
          ck::index_t BBlockTransferSrcVectorDim,
          ck::index_t BBlockTransferSrcScalarPerVector,
          ck::index_t BBlockTransferDstScalarPerVector_K1,
          bool BThreadTransferSrcResetCoordinateAfterRun,
          typename CThreadTransferSrcDstAccessOrder,
          ck::index_t CThreadTransferSrcDstVectorDim,
          ck::index_t CThreadTransferDstScalarPerVector,
          typename AGridStepHacks,
          typename BGridStepHacks,
          typename CGridStepHacks,
          typename AGridMoveSliceWindowStepHacks,
          typename BGridMoveSliceWindowStepHacks,
          bool CAccessOrderMRepeatNRepeat>
__host__ float driver_gemm_xdlops_v2r4(const FloatAB* p_a_grid,
                                       const FloatAB* p_b_grid,
                                       FloatC* p_c_grid,
                                       const ABK0MK1GridDesc& a_b_k0_m_k1_grid_desc,
                                       const BBK0NK1GridDesc& b_b_k0_n_k1_grid_desc,
                                       const CMNGridDesc& c_m_n_grid_desc,
                                       AGridStepHacks,
                                       BGridStepHacks,
                                       CGridStepHacks,
                                       AGridMoveSliceWindowStepHacks,
                                       BGridMoveSliceWindowStepHacks,
                                       ck::index_t nrepeat,
                                       const std::function<void()>* func)

{
    using namespace ck;

    constexpr auto I0 = Number<0>{};
    constexpr auto I1 = Number<1>{};
    constexpr auto I2 = Number<2>{};
    constexpr auto I3 = Number<3>{};

    using GridwiseGemm =
        GridwiseGemm_k0mk1_k0nk1_mn_xdlops_v2r4<BlockSize,
                                                FloatAB,
                                                FloatAcc,
                                                FloatC,
                                                CGlobalMemoryDataOperation,
                                                ABK0MK1GridDesc,
                                                BBK0NK1GridDesc,
                                                CMNGridDesc,
                                                MPerBlock,
                                                NPerBlock,
                                                KPerBlock,
                                                MPerXDL,
                                                NPerXDL,
                                                K1,
                                                MRepeat,
                                                NRepeat,
                                                ABlockTransferThreadSliceLengths_K0_M_K1,
                                                ABlockTransferThreadClusterLengths_K0_M_K1,
                                                ABlockTransferThreadClusterArrangeOrder,
                                                ABlockTransferSrcAccessOrder,
                                                ABlockTransferSrcVectorDim,
                                                ABlockTransferSrcScalarPerVector,
                                                ABlockTransferDstScalarPerVector_K1,
                                                AThreadTransferSrcResetCoordinateAfterRun,
                                                BBlockTransferThreadSliceLengths_K0_N_K1,
                                                BBlockTransferThreadClusterLengths_K0_N_K1,
                                                BBlockTransferThreadClusterArrangeOrder,
                                                BBlockTransferSrcAccessOrder,
                                                BBlockTransferSrcVectorDim,
                                                BBlockTransferSrcScalarPerVector,
                                                BBlockTransferDstScalarPerVector_K1,
                                                BThreadTransferSrcResetCoordinateAfterRun,
                                                CThreadTransferSrcDstAccessOrder,
                                                CThreadTransferSrcDstVectorDim,
                                                CThreadTransferDstScalarPerVector,
                                                AGridStepHacks,
                                                BGridStepHacks,
                                                CGridStepHacks,
                                                AGridMoveSliceWindowStepHacks,
                                                BGridMoveSliceWindowStepHacks,
                                                CAccessOrderMRepeatNRepeat>;

    {
        std::cout << "a_b_k0_m_k1_grid_desc{" << a_b_k0_m_k1_grid_desc.GetLength(I0) << ", "
                  << a_b_k0_m_k1_grid_desc.GetLength(I1) << ", "
                  << a_b_k0_m_k1_grid_desc.GetLength(I2) << ", "
                  << a_b_k0_m_k1_grid_desc.GetLength(I3) << "}" << std::endl;

        std::cout << "b_b_k0_n_k1_grid_desc{" << b_b_k0_n_k1_grid_desc.GetLength(I0) << ", "
                  << b_b_k0_n_k1_grid_desc.GetLength(I1) << ", "
                  << b_b_k0_n_k1_grid_desc.GetLength(I2) << ", "
                  << b_b_k0_n_k1_grid_desc.GetLength(I3) << "}" << std::endl;

        std::cout << "c_m_n_grid_desc{ " << c_m_n_grid_desc.GetLength(I0) << ", "
                  << c_m_n_grid_desc.GetLength(I1) << "}" << std::endl;
    }

    if(!GridwiseGemm::CheckValidity(a_b_k0_m_k1_grid_desc, b_b_k0_n_k1_grid_desc, c_m_n_grid_desc))
    {
        throw std::runtime_error(
            "wrong! GridwiseGemm_km_kn_m0m1n0n1_xdlops_v2r4 has invalid setting");
    }

    const auto c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc =
        GridwiseGemm::MakeCM0N0M1N1M2M3M4N2GridDescriptor(c_m_n_grid_desc);

    using CM0N0M1N1M2M3M4N2GridDesc = decltype(c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc);

    const auto KBatch = a_b_k0_m_k1_grid_desc.GetLength(I0);

    const auto c_block_cluster_adaptor =
        GridwiseGemm::MakeCBlockClusterAdaptor(c_m_n_grid_desc, KBatch);

    using CBlockClusterAdaptor = decltype(c_block_cluster_adaptor);

    const index_t grid_size = GridwiseGemm::CalculateGridSize(c_m_n_grid_desc, KBatch);
    {
        std::cout << "gridSize : " << grid_size << std::endl;
    }

    const auto kernel = kernel_gemm_xdlops_v2r4<GridwiseGemm,
                                                FloatAB,
                                                FloatC,
                                                remove_reference_t<ABK0MK1GridDesc>,
                                                remove_reference_t<BBK0NK1GridDesc>,
                                                remove_reference_t<CM0N0M1N1M2M3M4N2GridDesc>,
                                                remove_reference_t<CBlockClusterAdaptor>>;

#if CK_EXPERIMENTAL_PASS_TENSOR_DESCRIPTOR_BY_VALUE
    float ave_time = launch_and_time_kernel(kernel,
                                            nrepeat,
                                            dim3(grid_size),
                                            dim3(BlockSize),
                                            0,
                                            p_a_grid,
                                            p_b_grid,
                                            p_c_grid,
                                            a_b_k0_m_k1_grid_desc,
                                            b_b_k0_n_k1_grid_desc,
                                            c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc,
                                            c_block_cluster_adaptor);

#elif CK_EXPERIMENTAL_PASS_TENSOR_DESCRIPTOR_BY_VOID_POINTER
    DeviceMem a_b_k0_m_k1_grid_desc_dev_buf(sizeof(ABK0MK1GridDesc));
    DeviceMem b_b_k0_n_k1_grid_desc_dev_buf(sizeof(BBK0NK1GridDesc));
    DeviceMem c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc_dev_buf(sizeof(CM0N0M1N1M2M3M4N2GridDesc));
    DeviceMem c_block_cluster_adaptor_dev_buf(sizeof(CBlockClusterAdaptor));

    a_b_k0_m_k1_grid_desc_dev_buf.ToDevice(&a_b_k0_m_k1_grid_desc);
    b_b_k0_n_k1_grid_desc_dev_buf.ToDevice(&b_b_k0_n_k1_grid_desc);
    c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc_dev_buf.ToDevice(&c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc);
    c_block_cluster_adaptor_dev_buf.ToDevice(&c_block_cluster_adaptor);

    float ave_time = launch_time_and_out_call_kernel(
        kernel,
        nrepeat,
        dim3(grid_size),
        dim3(BlockSize),
        0,
        func,
        p_a_grid,
        p_b_grid,
        p_c_grid,
        cast_pointer_to_constant_address_space(a_b_k0_m_k1_grid_desc_dev_buf.GetDeviceBuffer()),
        cast_pointer_to_constant_address_space(b_b_k0_n_k1_grid_desc_dev_buf.GetDeviceBuffer()),
        cast_pointer_to_constant_address_space(
            c_m0_n0_m1_n1_m2_m3_m4_n2_grid_desc_dev_buf.GetDeviceBuffer()),
        cast_pointer_to_constant_address_space(c_block_cluster_adaptor_dev_buf.GetDeviceBuffer()));
#endif
    return ave_time;
}
#endif