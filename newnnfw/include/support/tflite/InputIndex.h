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

#ifndef __NNFW_SUPPORT_TFLITE_INPUT_INDEX_H__
#define __NNFW_SUPPORT_TFLITE_INPUT_INDEX_H__

namespace nnfw
{
namespace support
{
namespace tflite
{

class InputIndex
{
public:
  InputIndex(int index) : _index(index)
  {
    // DO NOTHING
  }

public:
  int asInt(void) const { return _index; }

private:
  int _index;
};

} // namespace tflite
} // namespace support
} // namespace nnfw

#endif // __NNFW_SUPPORT_TFLITE_INPUT_INDEX_H__