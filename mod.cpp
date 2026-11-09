/**
Copyright (c) 2026, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mkn/mod/init.hpp"  // IWYU pragma: keep

#include "mkn/kul/log.hpp"
#include "mkn/kul/yaml.hpp"

#include <hwloc.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/*

Added as compile flags by default, or written to a header with
  mod: ompi.hwloc{compile{header: path/to/file.hpp}}

-D_MKN_OMPI_HWLOC_L1_SIZE_=$SIZE  # CPU L1 cache size - 0 if not exists
-D_MKN_OMPI_HWLOC_L2_SIZE_=$SIZE  # CPU L2 cache size - 0 if not exists
-D_MKN_OMPI_HWLOC_L3_SIZE_=$SIZE  # CPU L3 cache size - 0 if not exists
-D_MKN_OMPI_HWLOC_L1_CL_SIZE_=$SIZE  # CPU L1 cache line size - 0 if not exists
-D_MKN_OMPI_HWLOC_L2_CL_SIZE_=$SIZE  # CPU L2 cache line size - 0 if not exists
-D_MKN_OMPI_HWLOC_L3_CL_SIZE_=$SIZE  # CPU L3 cache line size - 0 if not exists

*/

namespace mkn::ompi {

struct Cache {
  std::uint64_t size = 0;
  std::uint16_t line_size = 0;
};

class hwlocModule : public mkn::mod::Module {
 private:
  hwloc_topology_t topo;

 public:
  hwlocModule() {
    hwloc_topology_init(&topo);
    hwloc_topology_load(topo);
  }
  ~hwlocModule() { hwloc_topology_destroy(topo); }

  void compile(mkn::mod::Context& ctx, YAML::Node const& node) override {
    try {
      using namespace mkn::kul::yaml;
      if (node) Validator({NodeValidator("header")}).validate(node);

      auto const defines = cache_defines();
      if (node && node["header"])
        write_header(ctx, node["header"].Scalar(), defines);
      else
        for (auto const& [name, value] : defines)
          ctx.compilerState().add(
              mkn::mod::CompileArgInput{"-D" + name + "=" + std::to_string(value)});
    } catch (mkn::kul::Exception const& e) {
      KERR << e.stack();
    } catch (std::exception const& e) {
      KERR << e.what();
    } catch (...) {
      KERR << "UNKNOWN ERROR CAUGHT";
    }
  }

 private:
  using Defines = std::vector<std::pair<std::string, std::uint64_t>>;

  Defines cache_defines() {
    std::map<std::uint16_t, Cache> caches;
    std::function<void(hwloc_obj_t)> check = [&](hwloc_obj_t obj) -> void {
      for (std::size_t i = 0; i < obj->arity; i++) check(obj->children[i]);
      auto insert = [&](std::uint16_t const t) -> void {
        if (!caches.count(t)) caches[t].line_size = obj->attr->cache.linesize;
      };
      if (obj->type == HWLOC_OBJ_L3CACHE)
        insert(3);
      else if (obj->type == HWLOC_OBJ_L2CACHE)
        insert(2);
      else if (obj->type == HWLOC_OBJ_L1CACHE)
        insert(1);
    };

    check(hwloc_get_root_obj(topo));

    for (auto obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, 0); obj; obj = obj->parent) {
      if (obj->type == HWLOC_OBJ_L3CACHE) caches[3].size += obj->attr->cache.size;
      if (obj->type == HWLOC_OBJ_L2CACHE) caches[2].size += obj->attr->cache.size;
      if (obj->type == HWLOC_OBJ_L1CACHE) caches[1].size += obj->attr->cache.size;
    }

    Defines defines;
    for (std::uint16_t i = 1; i <= 3; i++) {
      auto const prefix = "_MKN_OMPI_HWLOC_L" + std::to_string(i);
      defines.emplace_back(prefix + "_SIZE_", caches[i].size);
      defines.emplace_back(prefix + "_CL_SIZE_", caches[i].line_size);
    }
    return defines;
  }

  // Relative paths are under the project build directory, which is then added
  // as a private include path, so the header is included by the given path.
  // Only rewritten when the contents change, to not trigger rebuilds.
  static void write_header(mkn::mod::Context& ctx, std::string const& path,
                           Defines const& defines) {
    namespace fs = std::filesystem;
    fs::path file{path}, inc;
    if (file.is_absolute())
      inc = file.parent_path();
    else {
      auto const buildDir = ctx.state().get("buildDir");
      if (!buildDir) KEXCEPT(mkn::mod::Exception, "ompi.hwloc: no build directory for header");
      inc = *buildDir;
      file = inc / file;
    }

    std::stringstream ss;
    ss << "#ifndef _MKN_OMPI_HWLOC_DEFINES_\n#define _MKN_OMPI_HWLOC_DEFINES_\n\n";
    for (auto const& [name, value] : defines) ss << "#define " << name << " " << value << "\n";
    ss << "\n#endif  // _MKN_OMPI_HWLOC_DEFINES_\n";
    auto const contents = ss.str();

    std::string existing;
    if (std::ifstream in{file}; in)
      existing.assign(std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{});
    if (existing != contents) {
      fs::create_directories(file.parent_path());
      std::ofstream{file} << contents;
    }

    ctx.compilerState().add(mkn::mod::IncludeInput{inc.string(), false});
  }
};

}  // namespace mkn::ompi

extern "C" MKN_KUL_PUBLISH mkn::mod::Module* maiken_module_construct() {
  return new mkn::ompi::hwlocModule;
}

extern "C" MKN_KUL_PUBLISH void maiken_module_destruct(mkn::mod::Module* p) { delete p; }
