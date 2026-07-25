#include "cuda/DeviceBuffer.h"
#include "Order.h"

#include <cstdint>

// -----------------------------------------------------------------------------
// Explicit Template Instantiations
// Ensures common template instantiations are compiled when compiling DeviceBuffer.cu
// -----------------------------------------------------------------------------

template class DeviceBuffer<int>;
template class DeviceBuffer<float>;
template class DeviceBuffer<double>;
template class DeviceBuffer<std::uint32_t>;
template class DeviceBuffer<std::uint64_t>;
template class DeviceBuffer<Order>;