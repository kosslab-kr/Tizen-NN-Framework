/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef NN_API_EX_SHIM_H
#define NN_API_EX_SHIM_H

#include "NeuralNetworksEx.h"
#include "NeuralNetworksLoadHelpers.h"

typedef int (*ANeuralNetworksModel_addOperationEx_fn)(
    ANeuralNetworksModel *model, ANeuralNetworksOperationTypeEx type,
    uint32_t inputCount, const uint32_t *inputs, uint32_t outputCount,
    const uint32_t *outputs);

/**
 * Add an extended operation to a model.
 *
 * @param model The model to be modified.
 * @param type The type of extended operation.
 * @param inputCount The number of entries in the inputs array.
 * @param inputs An array of indexes identifying each operand.
 * @param outputCount The number of entries in the outputs array.
 * @param outputs An array of indexes identifying each operand.
 *
 * The operands specified by inputs and outputs must have been
 * previously added by calls to {@link ANeuralNetworksModel_addOperand}.
 *
 * Attempting to modify a model once {@link ANeuralNetworksModel_finish} has
 * been
 * called will return an error.
 *
 * See {@link ANeuralNetworksModel} for information on multithreaded usage.
 *
 * @return ANEURALNETWORKS_NO_ERROR if successful.
 */

inline int ANeuralNetworksModel_addOperationEx(
    ANeuralNetworksModel *model, ANeuralNetworksOperationTypeEx type,
    uint32_t inputCount, const uint32_t *inputs, uint32_t outputCount,
    const uint32_t *outputs) {
  LOAD_FUNCTION(ANeuralNetworksModel_addOperationEx);
  EXECUTE_FUNCTION_RETURN(model, type, inputCount, inputs, outputCount,
                          outputs);
}

#endif // NN_API_EX_SHIM_H
