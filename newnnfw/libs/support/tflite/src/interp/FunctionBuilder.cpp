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

#include "support/tflite/interp/FunctionBuilder.h"

namespace nnfw
{
namespace support
{
namespace tflite
{
namespace interp
{

std::unique_ptr<::tflite::Interpreter> FunctionBuilder::build(void) const
{
  auto res = std::unique_ptr<::tflite::Interpreter>{new ::tflite::Interpreter};

  _fn(*res);

  return std::move(res);
}

} // namespace interp
} // namespace tflite
} // namespace support
} // namespace nnfw