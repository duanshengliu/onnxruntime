// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/cuda/math/clip.h"
#include "core/providers/cuda/math/clip_impl.h"

namespace onnxruntime {
namespace cuda {

ONNX_OPERATOR_VERSIONED_TYPED_KERNEL_EX(
    Clip,
    kOnnxDomain,
    6,
    10,
    float,
    kCudaExecutionProvider,
    (*KernelDefBuilder::Create())
        .TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
    Clip_6<float>);

ONNX_OPERATOR_VERSIONED_KERNEL_EX(
    Clip,
    kOnnxDomain,
    11, 11,
    kCudaExecutionProvider,
    (*KernelDefBuilder::Create())
        .TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
    Clip);

ONNX_OPERATOR_VERSIONED_KERNEL_EX(
    Clip,
    kOnnxDomain,
    12, 12,
    kCudaExecutionProvider,
    (*KernelDefBuilder::Create())
        .TypeConstraint("T", BuildKernelDefConstraints<float, double, MLFloat16, int8_t, uint8_t, int64_t, uint64_t>()),
    Clip);

ONNX_OPERATOR_KERNEL_EX(
    Clip,
    kOnnxDomain,
    13,
    kCudaExecutionProvider,
    (*KernelDefBuilder::Create())
        .TypeConstraint("T", BuildKernelDefConstraints<float, double, MLFloat16, int8_t, uint8_t, int64_t, uint64_t>()),
    Clip);

template <typename T>
Status Clip_6<T>::ComputeInternal(OpKernelContext* ctx) const {
  const Tensor& X = *ctx->Input<Tensor>(0);
  const TensorShape& input_shape{X.Shape()};
  Tensor* Y = ctx->Output(0, input_shape);
  const size_t count = input_shape.Size();
  if (count > 0) {
    auto* y_data = Y->MutableData<T>();
    const auto* x_data = X.Data<T>();
    ClipImpl<T>(Stream(ctx), x_data, y_data, nullptr, nullptr, this->min_, this->max_, count);
  }
  return Status::OK();
}

template <typename T>
struct Clip::ComputeImpl {
  void operator()(cudaStream_t stream, const Tensor* X, const Tensor* min, const Tensor* max, Tensor* Y) const {
    constexpr T min_default = std::numeric_limits<T>::lowest();
    constexpr T max_default = std::numeric_limits<T>::max();

    const T* min_data = nullptr;
    const T* max_data = nullptr;
    // 1-2 Input on CPU
    if (min) {
      ORT_ENFORCE(min->Shape().IsScalar(), "min should be a scalar.");
      min_data = min->Data<T>();
    }

    if (max) {
      ORT_ENFORCE(max->Shape().IsScalar(), "max should be a scalar.");
      max_data = max->Data<T>();
    }

    const size_t count = X->Shape().Size();
    if (count > 0) {
      auto* y_data = Y->MutableData<T>();
      const auto* x_data = X->Data<T>();
      ClipImpl<T>(stream, x_data, y_data, min_data, max_data, min_default, max_default, count);
    }
  }
};

Status Clip::ComputeInternal(OpKernelContext* ctx) const {
  const auto* X = ctx->Input<Tensor>(0);
  const auto* min = ctx->Input<Tensor>(1);
  const auto* max = ctx->Input<Tensor>(2);
  Tensor* Y = ctx->Output(0, X->Shape());

  utils::MLTypeCallDispatcher<float, double, MLFloat16, int8_t, uint8_t, int64_t, uint64_t>
      t_disp(X->GetElementType());

  t_disp.Invoke<ComputeImpl>(Stream(ctx), X, min, max, Y);

  return Status::OK();
}

}  // namespace cuda
}  // namespace onnxruntime
