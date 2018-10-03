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

#ifndef __INTERNAL_NNAPI_FEATURE_UTILS_H__
#define __INTERNAL_NNAPI_FEATURE_UTILS_H__

#include "util/feature/Shape.h"

namespace internal
{
namespace nnapi
{
namespace feature
{

inline uint32_t index_of(const ::nnfw::util::feature::Shape &shape, uint32_t ch, uint32_t row,
                         uint32_t col)
{
  uint32_t res = 0;

  // NNAPI uses NHWC ordering
  res += row * shape.W * shape.C;
  res += col * shape.C;
  res += ch;

  return res;
}

inline uint32_t index_of(const ::nnfw::util::feature::Shape &shape, uint32_t batch, uint32_t ch,
                         uint32_t row, uint32_t col)
{
  uint32_t res = 0;

  // NNAPI uses NHWC ordering
  res += batch * shape.H * shape.W * shape.C;
  res += row * shape.W * shape.C;
  res += col * shape.C;
  res += ch;

  return res;
}

} // namespace feature
} // namespace nnapi
} // namespace internal

#endif // __INTERNAL_NNAPI_FEATURE_UTILS_H__
