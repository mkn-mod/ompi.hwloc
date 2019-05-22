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

Compile/Link phase module

## Prerequisites
  [maiken](https://github.com/Dekken/maiken)

## Usage

```yaml
mod:
- name: ompi.hwloc
```

## Building

  Windows cl:

    UNSUPPORTED


  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -d -l "-pthread -ldl"


## Testing

  Windows cl:

    UNSUPPORTED

  *nix gcc:

    mkn clean build -tSa "-O2 -fPIC" -dp test -l "-pthread -ldl" run

