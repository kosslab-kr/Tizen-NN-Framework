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

#ifndef __INTERNAL_NNAPI_KERNEL_READER_H__
#define __INTERNAL_NNAPI_KERNEL_READER_H__

#include "util/kernel/Shape.h"
#include "util/kernel/Reader.h"

namespace internal
{
namespace nnapi
{
namespace kernel
{

template <typename T> class Reader final : public nnfw::util::kernel::Reader<T>
{
public:
  // NOTE The parameter len denotes the number of bytes.
  Reader(const ::nnfw::util::kernel::Shape &shape, const T *ptr, size_t len)
      : _shape{shape}, _ptr{ptr}
  {
    assert(shape.N * shape.C * shape.H * shape.W * sizeof(T) == len);
  }

public:
  const nnfw::util::kernel::Shape &shape(void) const { return _shape; }

public:
  T at(uint32_t nth, uint32_t ch, uint32_t row, uint32_t col) const override
  {
    // NNAPI uses NHWC ordering
    uint32_t index = 0;

    index += nth * _shape.H * _shape.W * _shape.C;
    index += row * _shape.W * _shape.C;
    index += col * _shape.C;
    index += ch;

    return _ptr[index];
  }

private:
  nnfw::util::kernel::Shape _shape;

private:
  const T *_ptr;
};

} // namespace kernel
} // namespace nnapi
} // namespace internal

#endif // __INTERNAL_NNAPI_KERNEL_READER_H__