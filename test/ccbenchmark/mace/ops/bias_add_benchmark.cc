// Copyright 2018 The MACE Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mace/benchmark_utils/test_benchmark.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

namespace {
template <DeviceType D, typename T>
void BiasAdd(int iters, int batch, int channels, int height, int width) {
  mace::testing::StopTiming();

  OpsTestNet net;

  // Add input data
  if (D == DeviceType::CPU && DataTypeToEnum<T>::value != DT_UINT8) {
    net.AddRandomInput<D, T>("Input", {batch, channels, height, width});
  } else {
    net.AddRandomInput<D, T>("Input", {batch, height, width, channels});
  }
  net.AddRandomInput<D, T>("Bias", {channels}, true, true);
  net.GetTensor("Input")->SetScale(0.1);
  net.GetTensor("Bias")->SetScale(0.1);

  OpDefBuilder("BiasAdd", "BiasAddBM")
      .Input("Input")
      .Input("Bias")
      .AddIntArg("has_data_format", 1)
      .Output("Output")
      .AddIntArg("T", static_cast<int>(DataTypeToEnum<T>::value))
      .Finalize(net.NewOperatorDef());

  net.Setup(D);
  net.GetTensor("Output")->SetScale(0.1);

  // Warm-up
  for (int i = 0; i < 5; ++i) {
    net.Run();
  }

  mace::testing::StartTiming();
  while (iters--) {
    net.Run();
  }
}
}  // namespace

#define MACE_BM_BIAS_ADD_MACRO(N, C, H, W, TYPE, DEVICE)                  \
  static void MACE_BM_BIAS_ADD_##N##_##C##_##H##_##W##_##TYPE##_##DEVICE( \
      int iters) {                                                        \
    const int64_t tot = static_cast<int64_t>(iters) * N * C * H * W;      \
    mace::testing::BytesProcessed(tot *(sizeof(TYPE)));                   \
    BiasAdd<DEVICE, TYPE>(iters, N, C, H, W);                             \
  }                                                                       \
  MACE_BENCHMARK(MACE_BM_BIAS_ADD_##N##_##C##_##H##_##W##_##TYPE##_##DEVICE)

#ifdef MACE_ENABLE_QUANTIZE
#define MACE_BM_BIAS_ADD_Q8_MACRO(N, C, H, W)      \
  MACE_BM_BIAS_ADD_MACRO(N, C, H, W, uint8_t, CPU)
#else
#define MACE_BM_BIAS_ADD_Q8_MACRO(N, C, H, W)
#endif  // MACE_ENABLE_QUANTIZE
#ifdef MACE_ENABLE_BFLOAT16
#define MACE_BM_BIAS_ADD_BF16_MACRO(N, C, H, W)      \
  MACE_BM_BIAS_ADD_MACRO(N, C, H, W, BFloat16, CPU)
#else
#define MACE_BM_BIAS_ADD_BF16_MACRO(N, C, H, W)
#endif  // MACE_ENABLE_BFLOAT16
#ifdef MACE_ENABLE_OPENCL
#define MACE_BM_BIAS_ADD_GPU_MACRO(N, C, H, W)       \
  MACE_BM_BIAS_ADD_MACRO(N, C, H, W, float, GPU);    \
  MACE_BM_BIAS_ADD_MACRO(N, C, H, W, half, GPU)
#else
#define MACE_BM_BIAS_ADD_GPU_MACRO(N, C, H, W)
#endif  // MACE_ENABLE_OPENCL

#define MACE_BM_BIAS_ADD(N, C, H, W)                 \
  MACE_BM_BIAS_ADD_MACRO(N, C, H, W, float, CPU);    \
  MACE_BM_BIAS_ADD_Q8_MACRO(N, C, H, W);             \
  MACE_BM_BIAS_ADD_BF16_MACRO(N, C, H, W);           \
  MACE_BM_BIAS_ADD_GPU_MACRO(N, C, H, W)

MACE_BM_BIAS_ADD(1, 1, 512, 512);
MACE_BM_BIAS_ADD(1, 3, 128, 128);
MACE_BM_BIAS_ADD(1, 3, 512, 512);
MACE_BM_BIAS_ADD(1, 32, 112, 112);
MACE_BM_BIAS_ADD(1, 64, 256, 256);
MACE_BM_BIAS_ADD(1, 64, 512, 512);
MACE_BM_BIAS_ADD(1, 128, 56, 56);
MACE_BM_BIAS_ADD(1, 128, 256, 256);
MACE_BM_BIAS_ADD(1, 256, 14, 14);
MACE_BM_BIAS_ADD(1, 512, 14, 14);
MACE_BM_BIAS_ADD(1, 1024, 7, 7);
MACE_BM_BIAS_ADD(32, 1, 256, 256);
MACE_BM_BIAS_ADD(32, 3, 256, 256);

}  // namespace test
}  // namespace ops
}  // namespace mace
