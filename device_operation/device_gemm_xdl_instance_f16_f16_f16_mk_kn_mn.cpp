#include <stdlib.h>
#include "config.hpp"
#include "device_gemm_xdl.hpp"
#include "device_gemm_xdl_instance.hpp"

namespace ck {
namespace tensor_operation {
namespace device {
namespace device_gemm_xdl_instance {

using F16 = ck::half_t;
using F32 = float;

using Row = ck::tensor_layout::gemm::RowMajor;
using Col = ck::tensor_layout::gemm::ColumnMajor;

template <ck::index_t... Is>
using S = ck::Sequence<Is...>;

// Compilation parameters for a[m, k] * b[k, n] = c[m, n]
using device_gemm_xdl_instance_f16_f16_f16_mk_kn_mn = std::tuple<
    // clang-format off
        //##########| AData| BData| CData| AccData| ALayout| BLayout| CLayout| Block|  MPer|  NPer| K0Per| K1| MPer| NPer| MXdl| NXdl|  ABlockTransfer|  ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer| ABlockTransfer|  BBlockTransfer|  BBlockTransfer| BBlockTransfer| BBlockTransfer| BlockTransfer| BBlockTransfer| BBlockTransfer| CThreadTransfer| CThreadTransfer| ABlockLds| BBlockLds|
        //##########|  Type|  Type|  Type|    Type|        |        |        |  Size| Block| Block| Block|   |  XDL|  XDL|  Per|  Per|     ThreadSlice|   ThreadCluster|  ThreadCluster| SrcAccessOrder|   SrcVectorDim|      SrcScalar|      DstScalar|     ThreadSlice|   ThreadCluster|  ThreadCluster| SrcAccessOrder|  SrcVectorDim|      SrcScalar|      DstScalar| SrcDstVectorDim|       DstScalar| AddExtraM| AddExtraN|
        //##########|      |      |      |        |        |        |        |      |      |      |      |   |     |     | Wave| Wave| Lengths_K0_N_K1| Lengths_K0_M_K1|   ArrangeOrder|               |               |      PerVector|   PerVector_K1| Lengths_K0_N_K1| Lengths_K0_N_K1|   ArrangeOrder|               |              |      PerVector|   PerVector_K1|                |       PerVector|          |          |
        //##########|      |      |      |        |        |        |        |      |      |      |      |   |     |     |     |     |                |                |               |               |               |               |               |                |                |               |               |              |               |               |                |                |          |          |
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   256,   256,   128,     4,  8,   32,   32,    4,    2,      S<1, 4, 8>,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 2, 8>,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   256,   128,   256,     4,  8,   32,   32,    2,    4,      S<1, 2, 8>,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 4, 8>,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              4,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   128,   128,   128,     4,  8,   32,   32,    4,    2,      S<1, 4, 8>,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 4, 8>,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              4,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   256,   128,   128,     4,  8,   32,   32,    2,    2,      S<1, 2, 8>,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 2, 8>,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   128,   128,    64,     4,  8,   32,   32,    2,    2,      S<1, 4, 8>,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 2, 8>,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   128,    64,   128,     4,  8,   32,   32,    2,    2,      S<1, 2, 8>,     S<4, 32, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 4, 8>,     S<4, 32, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              4,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   256,   128,    64,     4,  8,   32,   32,    2,    1,      S<1, 2, 8>,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 1, 8>,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              1,              8,               7,               1,      true,      true>,
        DeviceGemmXdl<  F16,   F16,   F16,     F32,     Row,      Row,    Row,   256,    64,   128,     4,  8,   32,   32,    1,    2,      S<1, 1, 8>,     S<4, 64, 1>,     S<1, 0, 2>,     S<1, 0, 2>,              2,              8,              8,      S<1, 2, 8>,     S<4, 64, 1>,     S<0, 2, 1>,     S<0, 2, 1>,             1,              2,              8,               7,               1,      true,      true>
    // clang-format on
    >;

template <>
void add_device_gemm_xdl_instance<F16, F16, F16, Row, Row, Row>(
    std::vector<DeviceGemmPtr>& device_op_instances)
{
    using DeviceGemms = device_gemm_xdl_instance::device_gemm_xdl_instance_f16_f16_f16_mk_kn_mn;

    const auto device_gemms = DeviceGemms{};

    ck::static_for<0, std::tuple_size_v<DeviceGemms>, 1>{}([&](auto i) {
        using Gemm = remove_cvref_t<decltype(std::get<i>(device_gemms))>;

        auto gemm = Gemm{};

        device_op_instances.push_back(std::make_unique<Gemm>(gemm));
    });
}

} // namespace device_gemm_xdl_instance
} // namespace device
} // namespace tensor_operation
} // namespace ck