#include "internal/op/HashtableLookup.h"
#include "internal/op/NodeVisitor.h"

#include <cassert>

namespace internal
{
namespace tflite
{
namespace op
{
namespace HashtableLookup
{

void Node::accept(NodeVisitor &&v) const { v.visit(*this); }

} // namespace HashtableLookup
} // namespace op
} // namespace tflite
} // namespace internal

namespace internal
{
namespace tflite
{
namespace op
{
namespace HashtableLookup
{

Param::Param(uint32_t inputCount, const uint32_t *inputs, uint32_t outputCount,
             const uint32_t *outputs)
{
  assert(inputCount == 3 && outputCount == 2);

  output_index = outputs[0];
  hits_index = outputs[1];

  // Each input should be interpreted as follows:
  //
  //  0 -> Lookups Index
  //  1 -> Keys Index
  //  2 -> Values Index
  lookups_index = inputs[0];
  keys_index = inputs[1];
  values_index = inputs[2];
}

} // namespace HashtableLookup
} // namespace op
} // namespace tflite
} // namespace internal
