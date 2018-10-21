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

#include "Tanh.h"

#include <cassert>

#include "NodeVisitor.h"
#include "LowerInfo.h"

namespace neurun
{
namespace graph
{
namespace operation
{
namespace Tanh
{

void Node::accept(NodeVisitor &&v) const { v.visit(*this); }

Node::Node(const graph::operation::Node::InitParam &init_param)
{
		assert(init_param.input_count == 1 && init_param.output_count == 1);
		setInputs({init_param.inputs[0]});
		setOutputs({init_param.outputs[0]});
}

void Node::setInputs(const operand::IndexSet &indexes)
{
	assert(indexes.size() == 1);

	graph::operation::Node::setInputs(indexes);
}

void Node::setOutputs(const operand::IndexSet &indexes)
{
	assert(indexes.size() == 1);

	graph::operation::Node::setOutputs(indexes);
}

} // namespace Tanh
} // namespace operation
} // namespace graph
} // namespace neurun
