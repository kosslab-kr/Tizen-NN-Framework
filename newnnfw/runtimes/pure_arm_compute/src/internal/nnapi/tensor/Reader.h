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

#ifndef __INTERNAL_NNAPI_TENSOR_READER_H__
#define __INTERNAL_NNAPI_TENSOR_READER_H__

#include <vector>
#include "util/tensor/Reader.h"

namespace internal
{
namespace nnapi
{
namespace tensor
{

template <typename T> class Reader final : public nnfw::util::tensor::Reader<T>
{
public:
  // NOTE The parameter len denotes the number of bytes.
  Reader(const ::nnfw::util::tensor::Shape &shape, const T *ptr, size_t len)
      : _shape{shape}, _ptr{ptr}
  {
    assert(shape.element_nums() * sizeof(T) == len);
    initialize();
  }

public:
  const nnfw::util::tensor::Shape &shape(void) const { return _shape; }

public:
  T at(const nnfw::util::tensor::Index &index_nnapi) const override
  {
    uint32_t offset = 0;

    for (int i = 0; i < _shape.rank(); i++)
      offset += index_nnapi.at(i) * _stridess.at(i);

    return _ptr[offset];
  }

private:
  /*
  Assuming that shape is [d4, .. , d1] and data is stored at a pointer ptr,
  we need to calculate the offset of index [i4, .. i1] as follows:
  offset = i4 * (d3 * d2 * d1) +
           i3 * (d2 * d1) +
           i2 * (d1) +
           i1
  So (d4 * d3 * d2 * d1) or (d3 * d2 * d1) or (d2 * d1) happens whenever offset is calculate.
  To minimize this repetitive calculation,
  _stridess[n] contains _spape[n-1]*_spape[n-2]*_spape[0]
  */
  void initialize(void)
  {
    for (int r = 0; r < _shape.rank(); r++)
    {
      int elem_count = 1;
      for (int k = r + 1; k < _shape.rank(); k++)
        elem_count *= _shape.dim(k);
      _stridess.emplace_back(elem_count);
    }
  }

private:
  nnfw::util::tensor::Shape _shape;

private:
  const T *_ptr;
  std::vector<int32_t> _stridess;
};

} // namespace tensor
} // namespace nnapi
} // namespace internal

#endif // __INTERNAL_NNAPI_TENSOR_READER_H__
