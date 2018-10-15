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

#ifndef __TENSOR3D_SOURCE_H__
#define __TENSOR3D_SOURCE_H__

#include "internal/Source.h"

//
// This is memcpy() version of generic TensorSource for 3D tensor
//
#include <arm_compute/core/ITensor.h>
#include <arm_compute/core/Window.h>
#include <arm_compute/core/Helpers.h>

template <typename T> class Tensor3DSource final : public Source
{
public:
  Tensor3DSource(const nnfw::util::tensor::Shape &shape, const T *base, const size_t size)
      : _shape{shape}, _base{base}, _size{size}
  {
    // DO NOTHING
  }

public:
  void push(::arm_compute::ITensor &tensor) const override
  {
    using ::arm_compute::Window;
    using ::arm_compute::Iterator;
    using ::arm_compute::Coordinates;
    using ::arm_compute::execute_window_loop;

    Window window;

    window.use_tensor_dimensions(tensor.info()->tensor_shape(), ::arm_compute::Window::DimY);
    int32_t height_width = _shape.dim(1) * _shape.dim(2);
    int32_t width = _shape.dim(2);

    Iterator it(&tensor, window);
    execute_window_loop(window,
                        [&](const ::arm_compute::Coordinates &id) {
                          const auto z = id.z();
                          const auto y = id.y();
                          memcpy(it.ptr(), _base + z * height_width + y * width, width * sizeof(T));
                        },
                        it);
  }

private:
  const nnfw::util::tensor::Shape _shape;

private:
  const T *const _base;
  const size_t _size;
};

#endif // __TENSOR3D_SOURCE_H__
