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

#include "support/tflite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"

#include "support/tflite/interp/FlatBufferBuilder.h"
#include "support/tflite/Diff.h"

#include <iostream>
#include <stdexcept>

using namespace tflite;
using namespace tflite::ops::builtin;

int main(const int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "nnapi_test\n\n";
    std::cerr << "Usage: " << argv[0] << " <.tflite>\n\n";
    return 1;
  }

  const auto filename = argv[1];

  StderrReporter error_reporter;

  auto model = FlatBufferModel::BuildFromFile(filename, &error_reporter);

  const nnfw::support::tflite::interp::FlatBufferBuilder builder(*model);

  try
  {
    return RandomTestRunner::make(0).run(builder);
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
