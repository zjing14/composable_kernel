#ifndef CK_TRANSFORM_BACKWARD_WEIGHT_CONVOLUTION_INTO_GEMM_V4R4R5_NHWC_KYXC_NHWK_HPP
#define CK_TRANSFORM_BACKWARD_WEIGHT_CONVOLUTION_INTO_GEMM_V4R4R5_NHWC_KYXC_NHWK_HPP

#include "common_header.hpp"
#include "tensor_descriptor.hpp"
#include "tensor_descriptor_helper.hpp"

namespace ck {

// A: out
// B: in
// C: wei
// GemmM = K
// GemmN = Y * X * C
// GemmKTotal = N * Ho * Wo
template <typename... In,
          typename... Wei,
          typename... Out,
          typename ConvStrides,
          typename ConvDilations,
          typename InLeftPads,
          typename InRightPads,
          index_t GemmK1Value,
          typename GemmKBatchType>
__host__ __device__ constexpr auto
transform_backward_weight_convolution_into_gemm_v4r4r5_nhwc_kyxc_nhwk_pad(
    const TensorDescriptor<In...>& in_n_hi_wi_c_grid_desc,
    const TensorDescriptor<Wei...>& wei_k_y_x_c_grid_desc,
    const TensorDescriptor<Out...>& out_n_ho_wo_k_grid_desc,
    const ConvStrides& conv_strides,
    const ConvDilations& conv_dilations,
    const InLeftPads& in_left_pads,
    const InRightPads& in_right_pads,
    Number<GemmK1Value>,
    GemmKBatchType GemmKBatch)
{
    constexpr auto I0 = Number<0>{};
    constexpr auto I1 = Number<1>{};
    constexpr auto I2 = Number<2>{};
    constexpr auto I3 = Number<3>{};

    constexpr auto GemmK1 = Number<GemmK1Value>{};

    const auto N = in_n_hi_wi_c_grid_desc.GetLength(I0);
    const auto C = in_n_hi_wi_c_grid_desc.GetLength(I3);
    const auto K = out_n_ho_wo_k_grid_desc.GetLength(I3);

    const auto Hi = in_n_hi_wi_c_grid_desc.GetLength(I1);
    const auto Wi = in_n_hi_wi_c_grid_desc.GetLength(I2);

    const auto Ho = out_n_ho_wo_k_grid_desc.GetLength(I1);
    const auto Wo = out_n_ho_wo_k_grid_desc.GetLength(I2);

    const auto Y = wei_k_y_x_c_grid_desc.GetLength(I1);
    const auto X = wei_k_y_x_c_grid_desc.GetLength(I2);

    const auto ConvStrideH = conv_strides[I0];
    const auto ConvStrideW = conv_strides[I1];

    const auto ConvDilationH = conv_dilations[I0];
    const auto ConvDilationW = conv_dilations[I1];

    const auto InLeftPadH = in_left_pads[I0];
    const auto InLeftPadW = in_left_pads[I1];

    const auto InRightPadH = in_right_pads[I0];
    const auto InRightPadW = in_right_pads[I1];

    const auto GemmM      = K;
    const auto GemmN      = Y * X * C;
    const auto GemmKTotal = N * Ho * Wo;
    const auto GemmK      = GemmKTotal / GemmKBatch;
    const auto GemmK0     = GemmK / GemmK1;

    // A: output tensor
    const auto out_gemmktotal_gemmm_grid_desc =
        make_naive_tensor_descriptor_packed(make_tuple(N * Ho * Wo, K));

    const auto out_gemmkbatch_gemmk0_gemmm_gemmk1_grid_desc = transform_tensor_descriptor(
        out_gemmktotal_gemmm_grid_desc,
        make_tuple(make_unmerge_transform(make_tuple(GemmKBatch, GemmK0, GemmK1)),
                   make_pass_through_transform(GemmM)),
        make_tuple(Sequence<0>{}, Sequence<1>{}),
        make_tuple(Sequence<0, 1, 3>{}, Sequence<2>{}));

    // B: input tensor
    const auto in_n_hip_wip_c_grid_desc = transform_tensor_descriptor(
        in_n_hi_wi_c_grid_desc,
        make_tuple(make_pass_through_transform(N),
                   make_pad_transform(Hi, InLeftPadH, InRightPadH),
                   make_pad_transform(Wi, InLeftPadW, InRightPadW),
                   make_pass_through_transform(C)),
        make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}, Sequence<3>{}),
        make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}, Sequence<3>{}));

    const auto in_n_y_ho_x_wo_c_grid_desc = transform_tensor_descriptor(
        in_n_hip_wip_c_grid_desc,
        make_tuple(make_pass_through_transform(N),
                   make_embed_transform(make_tuple(Y, Ho), make_tuple(ConvDilationH, ConvStrideH)),
                   make_embed_transform(make_tuple(X, Wo), make_tuple(ConvDilationW, ConvStrideW)),
                   make_pass_through_transform(C)),
        make_tuple(Sequence<0>{}, Sequence<1>{}, Sequence<2>{}, Sequence<3>{}),
        make_tuple(Sequence<0>{}, Sequence<1, 2>{}, Sequence<3, 4>{}, Sequence<5>{}));

    const auto in_gemmktotal_gemmn_grid_desc =
        transform_tensor_descriptor(in_n_y_ho_x_wo_c_grid_desc,
                                    make_tuple(make_merge_transform(make_tuple(Y, X, C)),
                                               make_merge_transform(make_tuple(N, Ho, Wo))),
                                    make_tuple(Sequence<1, 3, 5>{}, Sequence<0, 2, 4>{}),
                                    make_tuple(Sequence<1>{}, Sequence<0>{}));

    const auto in_gemmkbatch_gemmk0_gemmn_gemmk1_grid_desc = transform_tensor_descriptor(
        in_gemmktotal_gemmn_grid_desc,
        make_tuple(make_unmerge_transform(make_tuple(GemmKBatch, GemmK0, GemmK1)),
                   make_pass_through_transform(GemmN)),
        make_tuple(Sequence<0>{}, Sequence<1>{}),
        make_tuple(Sequence<0, 1, 3>{}, Sequence<2>{}));

    // C: weight tensor
    const auto wei_gemmm_gemmn_grid_desc =
        make_naive_tensor_descriptor_packed(make_tuple(K, Y * X * C));

    return make_tuple(out_gemmkbatch_gemmk0_gemmm_gemmk1_grid_desc,
                      in_gemmkbatch_gemmk0_gemmn_gemmk1_grid_desc,
                      wei_gemmm_gemmn_grid_desc);
}

} // namespace ck
#endif