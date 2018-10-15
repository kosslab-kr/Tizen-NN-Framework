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

#include "util/tensor/NonIncreasingStride.h"

#include <cassert>

namespace nnfw
{
namespace util
{
namespace tensor
{

uint32_t NonIncreasingStride::offset(const Index &index) const
{
  const size_t rank = _stride.size();

  assert(index.rank() == rank);

  uint32_t offset = 0;

  for (size_t axis = 0; axis < rank; ++axis)
  {
    offset += _stride.at(axis) * index.at(axis);
  }

  return offset;
}

} // namespace tensor
} // namespace util
} // namespace nnfw
