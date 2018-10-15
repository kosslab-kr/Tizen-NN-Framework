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

#ifndef __NEURUN_BACKEND_ACL_CL_OPERAND_OBJECT_H__
#define __NEURUN_BACKEND_ACL_CL_OPERAND_OBJECT_H__

#include <memory>
#include <arm_compute/core/CL/ICLTensor.h>

#include "backend/IObject.h"

namespace neurun
{
namespace backend
{
namespace acl_cl
{
namespace operand
{

class Object : public backend::operand::IObject
{
public:
  Object() = default;

public:
  Object(const std::shared_ptr<::arm_compute::ICLTensor> &tensor) : _tensor{tensor}
  {
    // DO NOTHING
  }

public:
  ::arm_compute::ICLTensor *ptr(void) const override { return _tensor.get(); }

private:
  std::shared_ptr<::arm_compute::ICLTensor> _tensor;

public:
  void access(const std::function<void(::arm_compute::ITensor &tensor)> &fn) const override;
};

} // namespace operand
} // namespace acl_cl
} // namespace backend
} // namespace neurun

#endif // __NEURUN_BACKEND_ACL_CL_OPERAND_OBJECT_H__
