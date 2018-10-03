/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include "Object.h"

#include <arm_compute/runtime/CL/CLScheduler.h>

namespace neurun
{
namespace backend
{
namespace acl_cl
{
namespace operand
{

void Object::access(const std::function<void(::arm_compute::ITensor &tensor)> &fn) const
{
  auto &queue = ::arm_compute::CLScheduler::get().queue();

  _tensor->map(queue);
  fn(*_tensor);
  _tensor->unmap(queue);
}

} // namespace operand
} // namespace acl_cl
} // namespace backend
} // namespace neurun
