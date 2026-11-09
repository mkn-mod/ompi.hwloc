# ompi.hwloc

** hwloc maiken module **

Adds the following define macros to a build for compile time CPU settings

size is in bytes by default

    -D_MKN_OMPI_HWLOC_L1_SIZE_=$SIZE  # CPU L1 cache size - 0 if not exists
    -D_MKN_OMPI_HWLOC_L2_SIZE_=$SIZE  # CPU L2 cache size - 0 if not exists
    -D_MKN_OMPI_HWLOC_L3_SIZE_=$SIZE  # CPU L3 cache size - 0 if not exists
    -D_MKN_OMPI_HWLOC_L1_CL_SIZE_=$SIZE  # CPU L1 cache line size - 0 if not exists
    -D_MKN_OMPI_HWLOC_L2_CL_SIZE_=$SIZE  # CPU L2 cache line size - 0 if not exists
    -D_MKN_OMPI_HWLOC_L3_CL_SIZE_=$SIZE  # CPU L3 cache line size - 0 if not exists

Compile phase module

## Prerequisites
  [maiken](https://github.com/mkn/mkn)

## Usage

```yaml
mod:
- name: ompi.hwloc
```

The defines are added as compile flags by default. To write them to a header instead:

```yaml
mod: |
  ompi.hwloc{compile{header: mkn/ompi/hwloc.hpp}}
```

```cpp
#include "mkn/ompi/hwloc.hpp"
```

A relative `header` path is created under the project build directory, which is added as a private
include path. An absolute path has its parent directory added instead. The header is only rewritten
when its contents change.

## Building

    mkn clean build -dtOa "-std=c++20 -fPIC"

## Testing

    mkn clean build run -dtOp test -a "-std=c++20 -fPIC"
    mkn clean build run -dtOp test_header -a "-std=c++20 -fPIC"
