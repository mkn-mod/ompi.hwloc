#if defined(MKN_OMPI_HWLOC_TEST_HEADER)
#if defined(_MKN_OMPI_HWLOC_L1_SIZE_)
#error "ompi.hwloc header mode should not add compile flags"
#endif
#include "mkn/ompi/hwloc.hpp"
#endif

#include <cstdio>

#if !defined(_MKN_OMPI_HWLOC_L1_SIZE_) || !defined(_MKN_OMPI_HWLOC_L1_CL_SIZE_) || \
    !defined(_MKN_OMPI_HWLOC_L2_SIZE_) || !defined(_MKN_OMPI_HWLOC_L2_CL_SIZE_) || \
    !defined(_MKN_OMPI_HWLOC_L3_SIZE_) || !defined(_MKN_OMPI_HWLOC_L3_CL_SIZE_)
#error "ompi.hwloc module defines missing"
#endif

int main() {
  std::printf("L1 %llu/%llu\n", 1ull * _MKN_OMPI_HWLOC_L1_SIZE_, 1ull * _MKN_OMPI_HWLOC_L1_CL_SIZE_);
  std::printf("L2 %llu/%llu\n", 1ull * _MKN_OMPI_HWLOC_L2_SIZE_, 1ull * _MKN_OMPI_HWLOC_L2_CL_SIZE_);
  std::printf("L3 %llu/%llu\n", 1ull * _MKN_OMPI_HWLOC_L3_SIZE_, 1ull * _MKN_OMPI_HWLOC_L3_CL_SIZE_);
  return 0;
}
