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

/*
 * Copyright (c) 2018 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __CONV2D_IO_ACCESSOR_H__
#define __CONV2D_IO_ACCESSOR_H__

#include <arm_compute/graph/ITensorAccessor.h>
#include <arm_compute/runtime/CL/CLFunctions.h>

#include "types.h"

class InputAccessor : public arm_compute::graph::ITensorAccessor
{
public:
    InputAccessor(const float* inputData, const Shape& inputShape);
    InputAccessor(InputAccessor&&) = default;

    // Inherited methods overriden:
    bool access_tensor(arm_compute::ITensor& tensor) override;

private:
    const float* _inputData;
    const Shape& _inputShape;
};

class WeightAccessor : public arm_compute::graph::ITensorAccessor
{
public:
    WeightAccessor(const float* filterData, const Shape& filterShape);
    WeightAccessor(WeightAccessor&&) = default;

    // Inherited methods overriden:
    bool access_tensor(arm_compute::ITensor& tensor) override;

private:
    const float* _filterData;
    const Shape& _filterShape;
};

class BiasAccessor : public arm_compute::graph::ITensorAccessor
{
public:
    BiasAccessor(const float* biasData, const Shape& biasShape);
    BiasAccessor(BiasAccessor&&) = default;

    // Inherited methods overriden:
    bool access_tensor(arm_compute::ITensor& tensor) override;

private:
    const float* _biasData;
    const Shape& _biasShape;
};

class OutputAccessor : public arm_compute::graph::ITensorAccessor
{
public:
    OutputAccessor(float* outputData, const Shape& outputShape);
    OutputAccessor(OutputAccessor&&) = default;

    // Inherited methods overriden:
    bool access_tensor(arm_compute::ITensor& tensor) override;

private:
    float* _outputData;
    const Shape& _outputShape;
};

#endif // __CONV2D_IO_ACCESSOR_H__