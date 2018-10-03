/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 * Copyright (c) 2016-2018 ARM Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "arm_compute/core/CL/kernels/CLPixelWiseDivisionKernel.h"

#include "arm_compute/core/CL/CLHelpers.h"
#include "arm_compute/core/CL/CLKernelLibrary.h"
#include "arm_compute/core/CL/CLKernelLibraryEx.h"
#include "arm_compute/core/CL/ICLTensor.h"
#include "arm_compute/core/CL/OpenCL.h"
#include "arm_compute/core/Error.h"
#include "arm_compute/core/Helpers.h"
#include "arm_compute/core/TensorInfo.h"
#include "arm_compute/core/Validate.h"
#include "arm_compute/core/Window.h"

#include <cmath>
#include <cstdlib>
#include <set>
#include <string>

using namespace arm_compute;

namespace
{
constexpr unsigned int num_elems_processed_per_iteration = 16;

Status validate_arguments(const ITensorInfo *input1, const ITensorInfo *input2,
                          const ITensorInfo *output, float scale, ConvertPolicy overflow_policy,
                          RoundingPolicy rounding_policy)
{
  ARM_COMPUTE_UNUSED(overflow_policy);
  ARM_COMPUTE_UNUSED(rounding_policy);

  ARM_COMPUTE_RETURN_ERROR_ON_DATA_TYPE_CHANNEL_NOT_IN(input1, 1, DataType::U8, DataType::QS8,
                                                       DataType::QS16, DataType::S16, DataType::F16,
                                                       DataType::F32);
  ARM_COMPUTE_RETURN_ERROR_ON_DATA_TYPE_CHANNEL_NOT_IN(input2, 1, DataType::U8, DataType::QS8,
                                                       DataType::QS16, DataType::S16, DataType::F16,
                                                       DataType::F32);
  ARM_COMPUTE_RETURN_ERROR_ON_MSG(scale < 0, "Scale cannot be negative.");

  const TensorShape &out_shape =
      TensorShape::broadcast_shape(input1->tensor_shape(), input2->tensor_shape());

  ARM_COMPUTE_RETURN_ERROR_ON_MSG(out_shape.total_size() == 0,
                                  "Inputs are not broadcast compatible");
  ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_FIXED_POINT(input1, input2);

  if (is_data_type_fixed_point(input1->data_type()))
  {
    // All data types must be all QS8 or all QS16
    ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_DATA_TYPES(input1, input2);
    ARM_COMPUTE_RETURN_ERROR_ON_MSG(scale != 1,
                                    "Unsupported scaling factor for QS8/QS16. Scale must be 1.");
  }

  // Validate in case of configured output
  if (output->total_size() > 0)
  {
    ARM_COMPUTE_RETURN_ERROR_ON_DATA_TYPE_CHANNEL_NOT_IN(output, 1, DataType::U8, DataType::QS8,
                                                         DataType::QS16, DataType::S16,
                                                         DataType::F16, DataType::F32);
    ARM_COMPUTE_RETURN_ERROR_ON_MSG(
        output->data_type() == DataType::U8 &&
            (input1->data_type() != DataType::U8 || input2->data_type() != DataType::U8),
        "Output can only be U8 if both inputs are U8");
    ARM_COMPUTE_RETURN_ERROR_ON_MSG(
        detail::have_different_dimensions(out_shape, output->tensor_shape(), 0),
        "Wrong shape for output");
    ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_FIXED_POINT(input1, output);
    if (is_data_type_fixed_point(input1->data_type()))
    {
      ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_DATA_TYPES(input1, output);
    }
  }

  return Status{};
}

std::pair<Status, Window> validate_and_configure_window(ITensorInfo *input1, ITensorInfo *input2,
                                                        ITensorInfo *output)
{
  const std::pair<TensorShape, ValidRegion> broadcast_pair =
      ITensorInfo::broadcast_shape_and_valid_region(*input1, *input2);
  const TensorShape &out_shape = broadcast_pair.first;
  const ValidRegion &valid_region = broadcast_pair.second;

  // Auto initialize output if not initialized
  {
    set_shape_if_empty(*output, out_shape);

    if (input1->data_type() == DataType::S16 || input2->data_type() == DataType::S16)
    {
      set_format_if_unknown(*output, Format::S16);
    }
    else if (input1->data_type() == DataType::F32 || input2->data_type() == DataType::F32)
    {
      set_format_if_unknown(*output, Format::F32);
    }
  }

  Window win = calculate_max_window(valid_region, Steps(num_elems_processed_per_iteration));
  Window win_input1 = win.broadcast_if_dimension_le_one(*input1);
  Window win_input2 = win.broadcast_if_dimension_le_one(*input2);

  AccessWindowHorizontal input1_access(input1, 0, num_elems_processed_per_iteration);
  AccessWindowHorizontal input2_access(input2, 0, num_elems_processed_per_iteration);
  AccessWindowHorizontal output_access(output, 0, num_elems_processed_per_iteration);

  bool window_changed = update_window_and_padding(win_input1, input1_access) ||
                        update_window_and_padding(win_input2, input2_access) ||
                        update_window_and_padding(win, output_access);

  output_access.set_valid_region(win, valid_region);

  Status err = (window_changed)
                   ? ARM_COMPUTE_CREATE_ERROR(ErrorCode::RUNTIME_ERROR, "Insufficient Padding!")
                   : Status{};
  return std::make_pair(err, win);
}
} // namespace

CLPixelWiseDivisionKernel::CLPixelWiseDivisionKernel()
    : _input1(nullptr), _input2(nullptr), _output(nullptr)
{
}

void CLPixelWiseDivisionKernel::configure(const ICLTensor *input1, const ICLTensor *input2,
                                          ICLTensor *output, float scale,
                                          ConvertPolicy overflow_policy,
                                          RoundingPolicy rounding_policy)
{
  ARM_COMPUTE_ERROR_ON_NULLPTR(input1, input2, output);
  ARM_COMPUTE_ERROR_THROW_ON(validate_arguments(input1->info(), input2->info(), output->info(),
                                                scale, overflow_policy, rounding_policy));

  // Configure kernel window
  auto win_config = validate_and_configure_window(input1->info(), input2->info(), output->info());
  ARM_COMPUTE_ERROR_THROW_ON(win_config.first);

  _input1 = input1;
  _input2 = input2;
  _output = output;

  int scale_int = -1;
  // Extract sign, exponent and mantissa
  int exponent = 0;
  float normalized_mantissa = std::frexp(scale, &exponent);
  // Use int scaling if factor is equal to 1/2^n for 0 <= n <= 15
  // frexp returns 0.5 as mantissa which means that the exponent will be in the range of -1 <= e <=
  // 14
  // Moreover, it will be negative as we deal with 1/2^n
  if ((normalized_mantissa == 0.5f) && (-14 <= exponent) && (exponent <= 1))
  {
    // Store the positive exponent. We know that we compute 1/2^n
    // Additionally we need to subtract 1 to compensate that frexp used a mantissa of 0.5
    scale_int = std::abs(exponent - 1);
  }

  std::string data_type;
  std::string compute_type;
  // Check if it has float inputs and output
  if (is_data_type_float(input1->info()->data_type()) ||
      is_data_type_float(input2->info()->data_type()))
  {
    scale_int = -1;
    compute_type = (input1->info()->data_type() == DataType::F32 ||
                    input2->info()->data_type() == DataType::F32)
                       ? "float"
                       : "half";
    data_type = "DATA_TYPE_FLOAT";
  }
  else
  {
    if (input1->info()->data_type() == DataType::S16 ||
        input2->info()->data_type() == DataType::S16)
    {
      compute_type = "int";
    }
    else if (input1->info()->data_type() == DataType::QS8)
    {
      compute_type = "qs8";
    }
    else if (input1->info()->data_type() == DataType::QS16)
    {
      compute_type = "qs16";
    }
    else
    {
      compute_type = "ushort";
    }
    data_type = "DATA_TYPE_INT";
  }

  // Construct kernel name
  std::string kernel_name = "pixelwise_div";
  kernel_name += (scale_int >= 0) ? "_int" : "_float";

  // Set kernel build options
  std::set<std::string> build_opts;
  build_opts.emplace(
      (overflow_policy == ConvertPolicy::WRAP || is_data_type_float(output->info()->data_type()))
          ? "-DWRAP"
          : "-DSATURATE");
  build_opts.emplace((rounding_policy == RoundingPolicy::TO_ZERO) ? "-DROUND=_rtz"
                                                                  : "-DROUND=_rte");
  if (is_data_type_fixed_point(input1->info()->data_type()))
  {
    build_opts.emplace("-DFIXED_POINT_POSITION=" +
                       support::cpp11::to_string(input1->info()->fixed_point_position()));
  }
  build_opts.emplace("-DDATA_TYPE_IN1=" + get_cl_type_from_data_type(input1->info()->data_type()));
  build_opts.emplace("-DDATA_TYPE_IN2=" + get_cl_type_from_data_type(input2->info()->data_type()));
  build_opts.emplace("-DDATA_TYPE_OUT=" + get_cl_type_from_data_type(output->info()->data_type()));
  build_opts.emplace("-DDATA_TYPE_RES=" + compute_type);
  build_opts.emplace("-D" + data_type);

  // Create kernel
  _kernel =
      static_cast<cl::Kernel>(CLKernelLibraryEx::get().create_kernel(kernel_name, build_opts));

  // Set scale argument
  unsigned int idx = 3 * num_arguments_per_3D_tensor(); // Skip the inputs and output parameters

  if (scale_int >= 0)
  {
    _kernel.setArg(idx++, scale_int);
  }
  else
  {
    _kernel.setArg(idx++, scale);
  }

  ICLKernel::configure(win_config.second);
}

Status CLPixelWiseDivisionKernel::validate(const ITensorInfo *input1, const ITensorInfo *input2,
                                           const ITensorInfo *output, float scale,
                                           ConvertPolicy overflow_policy,
                                           RoundingPolicy rounding_policy)
{
  ARM_COMPUTE_ERROR_ON_NULLPTR(input1, input2, output);
  ARM_COMPUTE_RETURN_ON_ERROR(
      validate_arguments(input1, input2, output, scale, overflow_policy, rounding_policy));
  ARM_COMPUTE_RETURN_ON_ERROR(validate_and_configure_window(input1->clone().get(),
                                                            input2->clone().get(),
                                                            output->clone().get())
                                  .first);

  return Status{};
}

void CLPixelWiseDivisionKernel::run(const Window &window, cl::CommandQueue &queue)
{
  ARM_COMPUTE_ERROR_ON_UNCONFIGURED_KERNEL(this);
  ARM_COMPUTE_ERROR_ON_INVALID_SUBWINDOW(ICLKernel::window(), window);

  const TensorShape &in_shape1 = _input1->info()->tensor_shape();
  const TensorShape &in_shape2 = _input2->info()->tensor_shape();
  const TensorShape &out_shape = _output->info()->tensor_shape();

  bool can_collapse = true;
  if (std::min(in_shape1.total_size(), in_shape2.total_size()) > 1)
  {
    can_collapse =
        (std::min(in_shape1.num_dimensions(), in_shape2.num_dimensions()) > Window::DimZ);
    for (size_t d = Window::DimZ; can_collapse && (d < out_shape.num_dimensions()); ++d)
    {
      can_collapse = (in_shape1[d] == in_shape2[d]);
    }
  }

  bool has_collapsed = false;
  Window collapsed =
      can_collapse ? window.collapse_if_possible(ICLKernel::window(), Window::DimZ, &has_collapsed)
                   : window;

  const TensorShape &in_shape1_collapsed =
      has_collapsed ? in_shape1.collapsed_from(Window::DimZ) : in_shape1;
  const TensorShape &in_shape2_collapsed =
      has_collapsed ? in_shape2.collapsed_from(Window::DimZ) : in_shape2;

  Window slice = collapsed.first_slice_window_3D();
  Window slice_input1 = slice.broadcast_if_dimension_le_one(in_shape1_collapsed);
  Window slice_input2 = slice.broadcast_if_dimension_le_one(in_shape2_collapsed);

  do
  {
    unsigned int idx = 0;
    add_3D_tensor_argument(idx, _input1, slice_input1);
    add_3D_tensor_argument(idx, _input2, slice_input2);
    add_3D_tensor_argument(idx, _output, slice);
    enqueue(queue, *this, slice);

    collapsed.slide_window_slice_3D(slice_input1);
    collapsed.slide_window_slice_3D(slice_input2);
  } while (collapsed.slide_window_slice_3D(slice));
}

BorderSize CLPixelWiseDivisionKernel::border_size() const
{
  const unsigned int replicateSize =
      _output->info()->dimension(0) -
      std::min(_input1->info()->dimension(0), _input2->info()->dimension(0));
  const unsigned int border =
      std::min<unsigned int>(num_elems_processed_per_iteration - 1U, replicateSize);
  return BorderSize(0, border, 0, 0);
}