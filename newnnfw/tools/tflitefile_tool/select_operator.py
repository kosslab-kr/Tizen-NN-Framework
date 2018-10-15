#!/usr/bin/python
import os
import sys
import numpy

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tflite'))
sys.path.append(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '../../externals/flatbuffers/python'))

import flatbuffers
import tflite.Model
import tflite.SubGraph
import tflite.BuiltinOptions
import argparse


# Assume we use only main model in model file
# Get selected operators from file, and return operator index list
def GetOperatorList(oplist_file):
    lines = oplist_file.readlines()
    opcode_list = []

    for line in lines:
        words = line.split()
        for word in words:
            if word.isdigit():
                opcode_list.append(int(word))
            else:
                opcode_range = word.split('-')
                if ((len(opcode_range) == 2) and opcode_range[0].isdigit()
                        and opcode_range[1].isdigit()):
                    start = int(opcode_range[0])
                    end = int(opcode_range[1])
                    for num in range(start, end + 1):
                        opcode_list.append(int(num))
                else:
                    print("Error: Cannot get operator list")
                    print(
                        "Please pass operators as operator index or range list split by space and/or line"
                    )
                    exit(1)

    if len(opcode_list) == 0:
        print("No selected operator")
        exit(1)

    return opcode_list


def GenerateOperatorCodes(new_builder, sample_model, used_operators_dic):
    operator_code_num = sample_model.OperatorCodesLength()
    new_operator_code_list = []
    new_operator_code_string_list = {}

    if operator_code_num == 0:
        return 0

    # Create operator_code string
    for operator_code_idx in range(operator_code_num):
        if operator_code_idx in used_operators_dic:
            operator_code = sample_model.OperatorCodes(operator_code_idx)
            operator_code_string = operator_code.CustomCode()
            if (operator_code_string !=
                    "") and (not operator_code_string in new_operator_code_string_list):
                new_operator_code_string_list[
                    operator_code_string] = new_builder.CreateString(operator_code_string)

    # Create tables of operator_code
    for operator_code_idx in range(operator_code_num):
        if operator_code_idx in used_operators_dic:
            operator_code = sample_model.OperatorCodes(operator_code_idx)

            # Create operator_code table
            tflite.OperatorCode.OperatorCodeStart(new_builder)
            tflite.OperatorCode.OperatorCodeAddBuiltinCode(new_builder,
                                                           operator_code.BuiltinCode())

            new_operator_code_string = operator_code.CustomCode()
            if new_operator_code_string in new_operator_code_string_list:
                tflite.OperatorCode.OperatorCodeAddCustomCode(
                    new_builder, new_operator_code_string_list[new_operator_code_string])
            new_operator_code = tflite.OperatorCode.OperatorCodeEnd(new_builder)
            new_operator_code_list.append(new_operator_code)

    # Create operator_code vector
    new_operator_code_num = len(new_operator_code_list)
    tflite.Model.ModelStartOperatorCodesVector(new_builder, new_operator_code_num)
    for operator_code_idx in reversed(range(new_operator_code_num)):
        new_builder.PrependUOffsetTRelative(new_operator_code_list[operator_code_idx])

    return new_builder.EndVector(new_operator_code_num)


def GenerateQuantization(new_builder, selected_quantization):
    # Create min vector
    min_num = selected_quantization.MinLength()
    if min_num != 0:
        tflite.QuantizationParameters.QuantizationParametersStartMinVector(
            new_builder, min_num)
        for min_idx in reversed(range(min_num)):
            new_builder.PrependFloat32(selected_quantization.Min(min_idx))
        new_min = new_builder.EndVector(min_num)

    # Create max vector
    max_num = selected_quantization.MaxLength()
    if max_num != 0:
        tflite.QuantizationParameters.QuantizationParametersStartMaxVector(
            new_builder, max_num)
        for max_idx in reversed(range(max_num)):
            new_builder.PrependFloat32(selected_quantization.Max(max_idx))
        new_max = new_builder.EndVector(max_num)

    # Create scale vector
    scale_num = selected_quantization.ScaleLength()
    if scale_num != 0:
        tflite.QuantizationParameters.QuantizationParametersStartScaleVector(
            new_builder, scale_num)
        for scale_idx in reversed(range(scale_num)):
            new_builder.PrependFloat32(selected_quantization.Scale(scale_idx))
        new_scale = new_builder.EndVector(scale_num)

    # Create zero_point vector
    zeropoint_num = selected_quantization.ZeroPointLength()
    if zeropoint_num != 0:
        tflite.QuantizationParameters.QuantizationParametersStartScaleVector(
            new_builder, zeropoint_num)
        for zeropoint_idx in reversed(range(zeropoint_num)):
            new_builder.PrependFloat32(selected_quantization.ZeroPoint(zeropoint_idx))
        new_zeropoint = new_builder.EndVector(zeropoint_num)

    # Create quantization
    tflite.QuantizationParameters.QuantizationParametersStart(new_builder)
    if min_num != 0:
        tflite.QuantizationParameters.QuantizationParametersAddMin(new_builder, new_min)
    if max_num != 0:
        tflite.QuantizationParameters.QuantizationParametersAddMax(new_builder, new_max)
    if scale_num != 0:
        tflite.QuantizationParameters.QuantizationParametersAddScale(
            new_builder, new_scale)
    if zeropoint_num != 0:
        tflite.QuantizationParameters.QuantizationParametersAddZeroPoint(
            new_builder, new_zeropoint)

    return tflite.QuantizationParameters.QuantizationParametersEnd(new_builder)


def GenerateTensor(new_builder, selected_tensor, used_buffers_dic):

    # Create shape vector for tensor
    shape_num = selected_tensor.ShapeLength()
    tflite.Tensor.TensorStartShapeVector(new_builder, shape_num)
    if shape_num != 0:
        for shape_idx in reversed(range(shape_num)):
            new_builder.PrependInt32(selected_tensor.Shape(shape_idx))
    new_shape = new_builder.EndVector(shape_num)

    # Create tensor_type
    tensor_type = selected_tensor.Type()

    # Create input vector for tensor
    buffer_idx = selected_tensor.Buffer()
    new_buffer_idx = used_buffers_dic[buffer_idx]

    # Create name string
    name_string = selected_tensor.Name()
    if name_string != "":
        new_name = new_builder.CreateString(name_string)

    # Create quantization
    quantization = selected_tensor.Quantization()
    if quantization != 0:
        new_quantization = GenerateQuantization(new_builder, quantization)

    # Create tensor
    tflite.Tensor.TensorStart(new_builder)
    tflite.Tensor.TensorAddShape(new_builder, new_shape)
    tflite.Tensor.TensorAddType(new_builder, tensor_type)
    tflite.Tensor.TensorAddBuffer(new_builder, new_buffer_idx)
    if name_string != "":
        tflite.Tensor.TensorAddName(new_builder, new_name)
    if quantization != 0:
        tflite.Tensor.TensorAddQuantization(new_builder, new_quantization)

    return tflite.Tensor.TensorEnd(new_builder)


def GenerateTensors(new_builder, selected_subgraph, used_tensors_dic, used_buffers_dic):
    tensor_num = selected_subgraph.TensorsLength()
    new_tensor_list = []

    if tensor_num == 0:
        return 0

    for tensor_idx in range(tensor_num):
        if tensor_idx in used_tensors_dic:
            selected_tensor = selected_subgraph.Tensors(tensor_idx)
            new_tensor = GenerateTensor(new_builder, selected_tensor, used_buffers_dic)
            new_tensor_list.append(new_tensor)

    new_tensor_num = len(new_tensor_list)
    if new_tensor_num == 0:
        return 0

    tflite.SubGraph.SubGraphStartTensorsVector(new_builder, new_tensor_num)
    for new_tensor in reversed(new_tensor_list):
        new_builder.PrependUOffsetTRelative(new_tensor)

    return new_builder.EndVector(new_tensor_num)


import tflite.Conv2DOptions
import tflite.DepthwiseConv2DOptions
import tflite.Pool2DOptions
import tflite.FullyConnectedOptions
import tflite.SoftmaxOptions
import tflite.ConcatenationOptions
import tflite.ReshapeOptions
import tflite.AddOptions
import tflite.SubOptions
import tflite.MulOptions
import tflite.DivOptions
import tflite.ResizeBilinearOptions
import tflite.StridedSliceOptions
import tflite.CastOptions
import tflite.TopKV2Options
import tflite.GatherOptions


def GenerateBuiltinOption(new_builder, selected_builtin_option, builtin_option_type):

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().Conv2DOptions:

        conv2d_options = tflite.Conv2DOptions.Conv2DOptions()
        conv2d_options.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.Conv2DOptions.Conv2DOptionsStart(new_builder)
        tflite.Conv2DOptions.Conv2DOptionsAddPadding(new_builder,
                                                     conv2d_options.Padding())
        tflite.Conv2DOptions.Conv2DOptionsAddStrideW(new_builder,
                                                     conv2d_options.StrideW())
        tflite.Conv2DOptions.Conv2DOptionsAddStrideH(new_builder,
                                                     conv2d_options.StrideH())
        tflite.Conv2DOptions.Conv2DOptionsAddFusedActivationFunction(
            new_builder, conv2d_options.FusedActivationFunction())
        return tflite.Conv2DOptions.Conv2DOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions(
    ).DepthwiseConv2DOptions:

        depthconv2d_option = tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptions()
        depthconv2d_option.Init(selected_builtin_option.Bytes,
                                selected_builtin_option.Pos)

        tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsStart(new_builder)
        tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsAddPadding(
            new_builder, depthconv2d_option.Padding())
        tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsAddStrideW(
            new_builder, depthconv2d_option.StrideW())
        tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsAddStrideH(
            new_builder, depthconv2d_option.StrideH())
        tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsAddDepthMultiplier(
            new_builder, depthconv2d_option.DepthMultiplier())
        tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsAddFusedActivationFunction(
            new_builder, depthconv2d_option.FusedActivationFunction())
        return tflite.DepthwiseConv2DOptions.DepthwiseConv2DOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().Pool2DOptions:

        pool2d_option = tflite.Pool2DOptions.Pool2DOptions()
        pool2d_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.Pool2DOptions.Pool2DOptionsStart(new_builder)
        tflite.Pool2DOptions.Pool2DOptionsAddPadding(new_builder, pool2d_option.Padding())
        tflite.Pool2DOptions.Pool2DOptionsAddStrideW(new_builder, pool2d_option.StrideW())
        tflite.Pool2DOptions.Pool2DOptionsAddStrideH(new_builder, pool2d_option.StrideH())
        tflite.Pool2DOptions.Pool2DOptionsAddFilterWidth(new_builder,
                                                         pool2d_option.FilterWidth())
        tflite.Pool2DOptions.Pool2DOptionsAddFilterHeight(new_builder,
                                                          pool2d_option.FilterHeight())
        tflite.Pool2DOptions.Pool2DOptionsAddFusedActivationFunction(
            new_builder, pool2d_option.FusedActivationFunction())
        return tflite.Pool2DOptions.Pool2DOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions(
    ).FullyConnectedOptions:

        fc_option = tflite.FullyConnectedOptions.FullyConnectedOptions()
        fc_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.FullyConnectedOptions.FullyConnectedOptionsStart(new_builder)
        tflite.FullyConnectedOptions.FullyConnectedOptionsAddFusedActivationFunction(
            new_builder, fc_option.FusedActivationFunction())
        return tflite.FullyConnectedOptions.FullyConnectedOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().SoftmaxOptions:

        softmax_option = tflite.SoftmaxOptions.SoftmaxOptions()
        softmax_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.SoftmaxOptions.SoftmaxOptionsStart(new_builder)
        tflite.SoftmaxOptions.SoftmaxOptionsAddBeta(new_builder, softmax_option.Beta())
        return tflite.SoftmaxOptions.SoftmaxOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().ConcatenationOptions:

        concat_option = tflite.ConcatenationOptions.ConcatenationOptions()
        concat_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.ConcatenationOptions.ConcatenationOptionsStart(new_builder)
        tflite.ConcatenationOptions.ConcatenationOptionsAddAxis(
            new_builder, concat_option.Axis())
        tflite.ConcatenationOptions.ConcatenationOptionsAddFusedActivationFunction(
            new_builder, concat_option.FusedActivationFunction())
        return tflite.ConcatenationOptions.ConcatenationOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().ReshapeOptions:

        reshape_option = tflite.ReshapeOptions.ReshapeOptions()
        reshape_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        shape_num = reshape_option.NewShapeLength()
        if shape_num != 0:
            tflite.ReshapeOptions.ReshapeOptionsStartNewShapeVector(
                new_builder, shape_num)
            for new_shape_idx in reversed(range(shape_num)):
                new_shape_val = reshape_option.NewShape(new_shape_idx)
                new_builder.PrependInt32(new_shape_val)
            new_shape = new_builder.EndVector(shape_num)

        tflite.ReshapeOptions.ReshapeOptionsStart(new_builder)
        if shape_num != 0:
            tflite.ReshapeOptions.ReshapeOptionsAddNewShape(new_builder, new_shape)
        return tflite.ReshapeOptions.ReshapeOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().AddOptions:

        add_option = tflite.AddOptions.AddOptions()
        add_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.AddOptions.AddOptionsStart(new_builder)
        tflite.AddOptions.AddOptionsAddFusedActivationFunction(
            new_builder, add_option.FusedActivationFunction())
        return tflite.AddOptions.AddOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().SubOptions:

        sub_option = tflite.SubOptions.SubOptions()
        sub_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.SubOptions.SubOptionsStart(new_builder)
        tflite.SubOptions.SubOptionsAddFusedActivationFunction(
            new_builder, sub_option.FusedActivationFunction())
        return tflite.SubOptions.SubOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().MulOptions:

        mul_option = tflite.MulOptions.MulOptions()
        mul_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.MulOptions.MulOptionsStart(new_builder)
        tflite.MulOptions.MulOptionsAddFusedActivationFunction(
            new_builder, mul_option.FusedActivationFunction())
        return tflite.MulOptions.MulOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().DivOptions:

        div_option = tflite.DivOptions.DivOptions()
        div_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.DivOptions.DivOptionsStart(new_builder)
        tflite.DivOptions.DivOptionsAddFusedActivationFunction(
            new_builder, div_option.FusedActivationFunction())
        return tflite.DivOptions.DivOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions(
    ).ResizeBilinearOptions:

        resize_bilinear_option = tflite.ResizeBilinearOptions.ResizeBilinearOptions()
        resize_bilinear_option.Init(selected_builtin_option.Bytes,
                                    selected_builtin_option.Pos)

        tflite.ResizeBilinearOptions.ResizeBilinearOptionsStart(new_builder)
        tflite.ResizeBilinearOptions.ResizeBilinearOptionsAddAlignCorners(
            new_builder, resize_bilinear_option.AlignCorners())
        return tflite.ResizeBilinearOptions.ResizeBilinearOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().StridedSliceOptions:

        stride_slice_option = tflite.StridedSliceOptions.StridedSliceOptions()
        stride_slice_option.Init(selected_builtin_option.Bytes,
                                 selected_builtin_option.Pos)

        tflite.StridedSliceOptions.StridedSliceOptionsStart(new_builder)
        tflite.StridedSliceOptions.StridedSliceOptionsAddBeginMask(
            new_builder, stride_slice_option.BeginMask())
        tflite.StridedSliceOptions.StridedSliceOptionsAddEndMask(
            new_builder, stride_slice_option.EndMask())
        tflite.StridedSliceOptions.StridedSliceOptionsAddEllipsisMask(
            new_builder, stride_slice_option.EllipsisMask())
        tflite.StridedSliceOptions.StridedSliceOptionsAddNewAxisMask(
            new_builder, stride_slice_option.NewAxisMask())
        tflite.StridedSliceOptions.StridedSliceOptionsAddShrinkAxisMask(
            new_builder, stride_slice_option.ShrinkAxisMask())

        return tflite.StridedSliceOptions.StridedSliceOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().CastOptions:

        cast_option = tflite.CastOptions.CastOptions()
        cast_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.CastOptions.CastOptionsStart(new_builder)
        return tflite.CastOptions.CastOptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().TopKV2Options:

        topkv2_option = tflite.TopKV2Options.TopKV2Options()
        topkv2_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.TopKV2Options.TopKV2OptionsStart(new_builder)
        return tflite.TopKV2Options.TopKV2OptionsEnd(new_builder)

    if builtin_option_type == tflite.BuiltinOptions.BuiltinOptions().GatherOptions:

        gather_option = tflite.GatherOptions.GatherOptions()
        gather_option.Init(selected_builtin_option.Bytes, selected_builtin_option.Pos)

        tflite.GatherOptions.GatherOptionsStart(new_builder)
        tflite.GatherOptions.GatherOptionsAddAxis(new_builder, gather_option.Axis())
        return tflite.GatherOptions.GatherOptionsEnd(new_builder)

    # Cannot handle builtin option type yet
    return 0


def GenerateOperator(new_builder, selected_operator, used_tensors_dic,
                     used_operators_dic):

    # define opcode_index
    opcode_index = selected_operator.OpcodeIndex()
    new_opcode_index = used_operators_dic[opcode_index]

    # create input vector
    input_num = selected_operator.InputsLength()
    if input_num != 0:
        new_input_list = []
        tflite.Operator.OperatorStartInputsVector(new_builder, input_num)
        for input_idx in reversed(range(input_num)):
            input_tensor_idx = selected_operator.Inputs(input_idx)
            new_input_tensor_idx = used_tensors_dic[input_tensor_idx]
            new_builder.PrependInt32(new_input_tensor_idx)
            new_input_list.append(new_input_tensor_idx)
        new_input = new_builder.EndVector(input_num)

    # create output_vector
    output_num = selected_operator.OutputsLength()
    if output_num != 0:
        tflite.Operator.OperatorStartOutputsVector(new_builder, output_num)
        for output_idx in reversed(range(output_num)):
            output_tensor_idx = selected_operator.Outputs(output_idx)
            new_output_tensor_idx = used_tensors_dic[output_tensor_idx]
            new_builder.PrependInt32(new_output_tensor_idx)
        new_output = new_builder.EndVector(output_num)

    # Create builtin_option
    builtin_option_type = selected_operator.BuiltinOptionsType()
    if builtin_option_type != 0:
        selected_builtin_option = selected_operator.BuiltinOptions()
        new_builtin_option = GenerateBuiltinOption(new_builder, selected_builtin_option,
                                                   builtin_option_type)

    # Create custum option vector
    custom_option_num = selected_operator.CustomOptionsLength()
    if custom_option_num != 0:
        tflite.Operator.OperatorStartCustomOptionsVector(new_builder, custom_option_num)
        for custom_option_idx in reversed(range(custom_option_num)):
            new_builder.PrependUint8(selected_operator.CustomOptions(custom_option_idx))
        new_custom_option = new_builder.EndVector(custom_option_num)

    # Create custum option type
    custom_option_type = selected_operator.CustomOptionsFormat()

    # Create operator
    tflite.Operator.OperatorStart(new_builder)
    tflite.Operator.OperatorAddOpcodeIndex(new_builder, new_opcode_index)
    if input_num != 0:
        tflite.Operator.OperatorAddInputs(new_builder, new_input)
    if output_num != 0:
        tflite.Operator.OperatorAddOutputs(new_builder, new_output)
    tflite.Operator.OperatorAddBuiltinOptionsType(new_builder, builtin_option_type)
    if builtin_option_type != 0:
        tflite.Operator.OperatorAddBuiltinOptions(new_builder, new_builtin_option)
    if custom_option_num != 0:
        tflite.Operator.OperatorAddCustomOptions(new_builder, new_custom_option)
    tflite.Operator.OperatorAddCustomOptionsFormat(new_builder, custom_option_type)
    return tflite.Operator.OperatorEnd(new_builder)


def GenerateOperators(new_builder, selected_subgraph, opcode_list, used_tensors_dic,
                      used_operators_dic):
    operator_num = selected_subgraph.OperatorsLength()
    new_operator_list = []

    if operator_num == 0:
        return 0

    for operator_idx in range(operator_num):
        if operator_idx in opcode_list:
            selected_operator = selected_subgraph.Operators(operator_idx)
            new_operator = GenerateOperator(new_builder, selected_operator,
                                            used_tensors_dic, used_operators_dic)
            new_operator_list.append(new_operator)

    new_operator_num = len(new_operator_list)
    if new_operator_num == 0:
        return 0

    tflite.SubGraph.SubGraphStartOperatorsVector(new_builder, new_operator_num)
    for new_operator in reversed(new_operator_list):
        new_builder.PrependUOffsetTRelative(new_operator)

    return new_builder.EndVector(new_operator_num)


def GenerateSubgraph(new_builder, selected_subgraph, opcode_list, new_input_tensor,
                     new_output_tensor, used_tensors_dic, used_buffers_dic,
                     used_operators_dic):

    # Tensors
    tensors = GenerateTensors(new_builder, selected_subgraph, used_tensors_dic,
                              used_buffers_dic)

    # Create input vector for subgraph table
    new_input_tensor_num = len(new_input_tensor)
    if new_input_tensor_num != 0:
        tflite.SubGraph.SubGraphStartInputsVector(new_builder, new_input_tensor_num)
        for input_tensor_idx in reversed(new_input_tensor):
            new_input_tensor_idx = used_tensors_dic[input_tensor_idx]
            new_builder.PrependInt32(new_input_tensor_idx)
        new_inputs = new_builder.EndVector(new_input_tensor_num)

    # Create output vector for subgraph table
    new_output_tensor_num = len(new_output_tensor)
    if new_output_tensor_num != 0:
        tflite.SubGraph.SubGraphStartInputsVector(new_builder, new_output_tensor_num)
        for output_tensor_idx in reversed(new_output_tensor):
            new_output_tensor_idx = used_tensors_dic[output_tensor_idx]
            new_builder.PrependInt32(new_output_tensor_idx)
        new_outputs = new_builder.EndVector(new_output_tensor_num)

    # Operators
    operators = GenerateOperators(new_builder, selected_subgraph, opcode_list,
                                  used_tensors_dic, used_operators_dic)

    # Name
    subgraph_name = selected_subgraph.Name()
    have_name = False
    if subgraph_name != "":
        have_name = True
        new_subgraph_name = new_builder.CreateString(subgraph_name)

    tflite.SubGraph.SubGraphStart(new_builder)
    tflite.SubGraph.SubGraphAddTensors(new_builder, tensors)
    if new_input_tensor_num != 0:
        tflite.SubGraph.SubGraphAddInputs(new_builder, new_inputs)
    if new_output_tensor_num != 0:
        tflite.SubGraph.SubGraphAddOutputs(new_builder, new_outputs)
    tflite.SubGraph.SubGraphAddOperators(new_builder, operators)
    if have_name:
        tflite.SubGraph.SubGraphAddName(new_builder, new_subgraph_name)

    return tflite.SubGraph.SubGraphEnd(new_builder)


def GenerateSubgraphs(new_builder, sample_model, opcode_list, new_input_tensor,
                      new_output_tensor, used_tensors_dic, used_buffers_dic,
                      used_operators_dic):
    new_subgraph_list = []

    # We think only main graph
    selected_subgraph = sample_model.Subgraphs(0)
    new_subgraph = GenerateSubgraph(new_builder, selected_subgraph, opcode_list,
                                    new_input_tensor, new_output_tensor, used_tensors_dic,
                                    used_buffers_dic, used_operators_dic)
    new_subgraph_list.append(new_subgraph)

    new_subgraph_num = 1
    tflite.Model.ModelStartSubgraphsVector(new_builder, new_subgraph_num)
    for subgraph_idx in reversed(range(new_subgraph_num)):
        new_builder.PrependUOffsetTRelative(new_subgraph_list[subgraph_idx])

    return new_builder.EndVector(new_subgraph_num)


def GenerateBuffers(new_builder, sample_model, used_buffers_dic):
    buffer_num = sample_model.BuffersLength()
    new_buffer_data_list = {}
    new_buffer_list = []

    if buffer_num == 0:
        return 0

    # Create data vector for buffer table
    for buffer_idx in range(buffer_num):
        buffer = sample_model.Buffers(buffer_idx)
        buffer_length = buffer.DataLength()

        if (buffer_length != 0) and (buffer_idx in used_buffers_dic):
            tflite.Buffer.BufferStartDataVector(new_builder, buffer_length)
            for buffer_data_idx in reversed(range(buffer_length)):
                new_builder.PrependUint8(buffer.Data(buffer_data_idx))
            new_buffer = new_builder.EndVector(buffer_length)
            new_buffer_data_list[buffer_idx] = new_buffer

    # Create tables of buffer
    for buffer_idx in range(buffer_num):
        buffer = sample_model.Buffers(buffer_idx)

        if buffer_idx in used_buffers_dic:
            # Create buffer table
            tflite.Buffer.BufferStart(new_builder)
            if buffer.DataLength() != 0:
                tflite.Buffer.BufferAddData(new_builder, new_buffer_data_list[buffer_idx])
            new_buffer = tflite.Buffer.BufferEnd(new_builder)
            new_buffer_list.append(new_buffer)

    # Create buffer vector
    new_buffer_num = len(new_buffer_list)
    if new_buffer_num == 0:
        return 0

    tflite.Model.ModelStartBuffersVector(new_builder, new_buffer_num)
    for new_buffer_idx in reversed(range(new_buffer_num)):
        new_builder.PrependUOffsetTRelative(new_buffer_list[new_buffer_idx])

    return new_builder.EndVector(new_buffer_num)


def GenerateModel(new_builder, sample_model, opcode_list, new_input_tensors,
                  new_output_tensors, used_tensors_dic, used_buffers_dic,
                  used_operators_dic):
    # uint
    version = sample_model.Version()

    # pointer of operator code 'table' vector
    operator_codes = GenerateOperatorCodes(new_builder, sample_model, used_operators_dic)

    # subgraphs
    subgraphs = GenerateSubgraphs(new_builder, sample_model, opcode_list,
                                  new_input_tensors, new_output_tensors, used_tensors_dic,
                                  used_buffers_dic, used_operators_dic)

    # description
    description_string = new_builder.CreateString(sample_model.Description())

    # buffers
    buffers = GenerateBuffers(new_builder, sample_model, used_buffers_dic)

    # Generate model
    tflite.Model.ModelStart(new_builder)
    tflite.Model.ModelAddVersion(new_builder, version)
    tflite.Model.ModelAddOperatorCodes(new_builder, operator_codes)
    tflite.Model.ModelAddSubgraphs(new_builder, subgraphs)
    tflite.Model.ModelAddDescription(new_builder, description_string)
    tflite.Model.ModelAddBuffers(new_builder, buffers)

    return tflite.Model.ModelEnd(new_builder)


def Finish(new_builder, new_model):
    # Cusrom implementation: identifier
    # Python API don't support identifier input yet
    # Reference: Finish(self, rootTable)) in builder.py, Finish(uoffset_t root, const char *file_identifier, bool size_prefix) in flatbuffers.h
    new_builder.Prep(new_builder.minalign,
                     flatbuffers.number_types.UOffsetTFlags.bytewidth)

    new_builder.PrependByte(0x33)
    new_builder.PrependByte(0x4c)
    new_builder.PrependByte(0x46)
    new_builder.PrependByte(0x54)

    new_builder.PrependUOffsetTRelative(new_model)
    new_builder.finished = True
    return new_builder.Head()


def main(args):
    input_model_file = args.input_model
    oplist_file = args.opcode_list
    output_model_file = args.output_model

    # Parse operator list file
    opcode_list = GetOperatorList(oplist_file)

    # Get sample model and subgraph
    # We use only 1st subgraph
    sample_buf = input_model_file.read()
    sample_buf = bytearray(sample_buf)
    sample_model = tflite.Model.Model.GetRootAsModel(sample_buf, 0)
    sample_subgraph = sample_model.Subgraphs(0)

    # Collect used tensor & used operator
    used_tensors = []
    used_operators = []

    for opcode_idx in opcode_list:
        opcode = sample_subgraph.Operators(opcode_idx)
        for input_idx in range(opcode.InputsLength()):
            input_tensor_idx = opcode.Inputs(input_idx)
            if not input_tensor_idx in used_tensors:
                # default: same as input sample
                used_tensors.append(input_tensor_idx)

        for output_idx in range(opcode.OutputsLength()):
            output_tensor_idx = opcode.Outputs(output_idx)
            if not output_tensor_idx in used_tensors:
                # default: same as input sample
                used_tensors.append(output_tensor_idx)

        opcode_idx = opcode.OpcodeIndex()
        if not opcode_idx in used_operators:
            used_operators.append(opcode_idx)

    used_tensors.sort()
    used_operators.sort()

    # Collect used buffer
    # buffer[0] should be blank. So it should start from 1
    used_buffers = [0]

    for used_tensor in used_tensors:
        # key and value is same in prepare phase
        buf_idx = (sample_subgraph.Tensors(used_tensor)).Buffer()
        used_buffers.append(buf_idx)
    used_buffers.sort()

    # Assign new index for operator
    used_operators_dic = {}

    for new_operator_idx in range(len(used_operators)):
        sample_operator_idx = used_operators[new_operator_idx]
        used_operators_dic[sample_operator_idx] = new_operator_idx

    # Assign new index for tensor
    used_tensors_dic = {}

    for new_tensor_idx in range(len(used_tensors)):
        sample_tensor_idx = used_tensors[new_tensor_idx]
        used_tensors_dic[sample_tensor_idx] = new_tensor_idx

    # Assign new index for buffer
    used_buffers_dic = {}

    for new_buffer_idx in range(len(used_buffers)):
        sample_buffer_idx = used_buffers[new_buffer_idx]
        used_buffers_dic[sample_buffer_idx] = new_buffer_idx

    # Find input & output tensor in new model
    new_input_tensors = used_tensors[:]
    new_output_tensors = used_tensors[:]

    for opcode_idx in opcode_list:
        opcode = sample_subgraph.Operators(opcode_idx)
        for input_idx in range(opcode.InputsLength()):
            input_tensor_idx = opcode.Inputs(input_idx)
            if input_tensor_idx in new_output_tensors:
                new_output_tensors.remove(input_tensor_idx)
            if input_tensor_idx in new_input_tensors:
                matched_buffer_idx = sample_subgraph.Tensors(input_tensor_idx).Buffer()
                matched_buffer = sample_model.Buffers(matched_buffer_idx)
                if matched_buffer.DataLength() != 0:
                    new_input_tensors.remove(input_tensor_idx)

        for output_idx in range(opcode.OutputsLength()):
            output_tensor_idx = opcode.Outputs(output_idx)
            if output_tensor_idx in new_input_tensors:
                new_input_tensors.remove(output_tensor_idx)
            if output_tensor_idx in new_output_tensors:
                matched_buffer_idx = sample_subgraph.Tensors(output_tensor_idx).Buffer()
                matched_buffer = sample_model.Buffers(matched_buffer_idx)
                if matched_buffer.DataLength() != 0:
                    new_output_tensors.remove(input_tensor_idx)

    new_input_tensors_newidx = []
    new_output_tensors_newidx = []

    for input_tensor_idx in new_input_tensors:
        new_input_tensors_newidx.append(used_tensors_dic[input_tensor_idx])
    for output_tensor_idx in new_output_tensors:
        new_output_tensors_newidx.append(used_tensors_dic[output_tensor_idx])

    print("Input tensor(s): " + str(new_input_tensors_newidx))
    print("Output tensor(s): " + str(new_output_tensors_newidx))

    # Create new model file
    new_builder = flatbuffers.Builder(1024)

    new_model = GenerateModel(new_builder, sample_model, opcode_list, new_input_tensors,
                              new_output_tensors, used_tensors_dic, used_buffers_dic,
                              used_operators_dic)

    Finish(new_builder, new_model)
    new_buf = new_builder.Output()

    output_model_file.write(new_buf)


if __name__ == '__main__':
    # Define argument and read
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument(
        "input_model",
        type=argparse.FileType('rb'),
        help="input tflite model file to read")
    arg_parser.add_argument(
        "opcode_list",
        type=argparse.FileType('r'),
        help="text file including selected operator list")
    arg_parser.add_argument(
        "output_model", type=argparse.FileType('wb'), help="output tflite model file")
    args = arg_parser.parse_args()

    # Call main function
    main(args)
