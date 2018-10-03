/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"

#include "support/tflite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"
#include "tensorflow/contrib/lite/builtin_op_data.h"

#include "env.h"
#include "memory.h"
#include "util/environment.h"
#include "util/feature/Shape.h"

#include "support/tflite/Diff.h"
#include "support/tflite/Quantization.h"
#include "support/tflite/interp/FunctionBuilder.h"

#include <chrono>
#include <random>
#include <iostream>
#include <cassert>

using namespace tflite;
using namespace tflite::ops::builtin;

TEST(NNAPI_Quickcheck_relu_1, simple_test)
{
  int verbose = 0;
  int tolerance = 1;

  nnfw::util::env::IntAccessor("VERBOSE").access(verbose);
  nnfw::util::env::IntAccessor("TOLERANCE").access(tolerance);

#define INT_VALUE(NAME, VALUE) IntVar NAME##_Value(#NAME, VALUE);
#include "relu_1.lst"
#undef INT_VALUE

  const int32_t IFM_H = IFM_H_Value();
  const int32_t IFM_W = IFM_W_Value();

  // Set random seed
  int SEED = std::chrono::system_clock::now().time_since_epoch().count();

  nnfw::util::env::IntAccessor("SEED").access(SEED);

  // Initialize random number generator
  std::minstd_rand random(SEED);

  std::cout << "Configurations:" << std::endl;
#define PRINT_NEWLINE()     \
  {                         \
    std::cout << std::endl; \
  }
#define PRINT_VALUE(value)                                       \
  {                                                              \
    std::cout << "  " << #value << ": " << (value) << std::endl; \
  }
  PRINT_VALUE(SEED);
  PRINT_NEWLINE();

  PRINT_VALUE(IFM_H);
  PRINT_VALUE(IFM_W);
#undef PRINT_VALUE
#undef PRINT_NEWLINE

  const int32_t OFM_H = IFM_H;
  const int32_t OFM_W = IFM_W;

  auto setup = [&](Interpreter &interp) {
    // Comment from 'context.h'
    //
    // Parameters for asymmetric quantization. Quantized values can be converted
    // back to float using:
    //    real_value = scale * (quantized_value - zero_point);
    //
    // Q: Is this necessary?
    TfLiteQuantizationParams quantization = make_default_quantization();

    // On AddTensors(N) call, T/F Lite interpreter creates N tensors whose index is [0 ~ N)
    interp.AddTensors(2);

    // Configure Output Tensor
    interp.SetTensorParametersReadWrite(0, kTfLiteFloat32 /* type */, "output" /* name */,
                                        {OFM_H, OFM_W} /* dims */, quantization);

    // Configure Input Tensor
    interp.SetTensorParametersReadWrite(1, kTfLiteFloat32 /* type */, "input" /* name */,
                                        {IFM_H, IFM_W} /* dims */, quantization);

    // Add ReLU Node
    // Run ReLU and store its result into Tensor #0
    //  - Read IFM from Tensor #1
    interp.AddNodeWithParameters({1}, {0}, nullptr, 0, nullptr,
                                 BuiltinOpResolver().FindOp(BuiltinOperator_RELU, 1));

    // Set Tensor #1 as Input #0, and Tensor #0 as Output #0
    interp.SetInputs({1});
    interp.SetOutputs({0});
  };

  const nnfw::support::tflite::interp::FunctionBuilder builder(setup);

  RandomTestParam param;

  param.verbose = verbose;
  param.tolerance = tolerance;

  int res = RandomTestRunner{SEED, param}.run(builder);

  EXPECT_EQ(res, 0);
}
