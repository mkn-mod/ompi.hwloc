/**
Copyright (c) 2013, Philip Deegan.
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
#include "maiken/module/init.hpp"
#include "mkn/kul/string.hpp"

#include <hwloc.h>
#include <iostream>
#include <functional>
#include <unordered_set>
#include <unordered_map>


/*

-D_MKN_OMPI_HWLOC_L1_SIZE_=$SIZE  # CPU L1 cache size - 0 if not exists
-D_MKN_OMPI_HWLOC_L2_SIZE_=$SIZE  # CPU L2 cache size - 0 if not exists
-D_MKN_OMPI_HWLOC_L3_SIZE_=$SIZE  # CPU L3 cache size - 0 if not exists
-D_MKN_OMPI_HWLOC_L1_CL_SIZE_=$SIZE  # CPU L1 cache line size - 0 if not exists
-D_MKN_OMPI_HWLOC_L2_CL_SIZE_=$SIZE  # CPU L2 cache line size - 0 if not exists
-D_MKN_OMPI_HWLOC_L3_CL_SIZE_=$SIZE  # CPU L3 cache line size - 0 if not exists

*/

namespace mkn {
namespace ompi {

class Cache{
  public:
    uint64_t size;
    uint16_t line_size;
};

class hwlocModule : public maiken::Module {
 private:
   hwloc_topology_t topo;
 public:
  hwlocModule(){
    hwloc_topology_init(&topo);
    hwloc_topology_load(topo);
  }
  ~hwlocModule(){

    hwloc_topology_destroy(topo);
  }
  void init(maiken::Application& a, YAML::Node const& node)
      KTHROW(std::exception) override {
  }

  void compile(maiken::Application& a, YAML::Node const& node) KTHROW(std::exception) override {
    try {
      hwloc_obj_t obj;
      std::unordered_map<uint16_t, Cache> caches;
      std::function<void(hwloc_obj_t , size_t)> check = [&](hwloc_obj_t obj, size_t depth) -> void {
        for (size_t i = 0; i < obj->arity; i++) check(obj->children[i], depth + 1);
        auto insert = [&](const uint16_t t) -> void {
          if(!caches.count(t))
            caches.emplace(t, Cache()).first->second.line_size = obj->attr->cache.linesize;
        };
        if (obj->type == HWLOC_OBJ_L3CACHE) insert(3);
        else
        if (obj->type == HWLOC_OBJ_L2CACHE) insert(2);
        else
        if (obj->type == HWLOC_OBJ_L1CACHE) insert(1);

      };

      check(hwloc_get_root_obj(topo), 0l);

      for (obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, 0); obj; obj = obj->parent){
        if (obj->type == HWLOC_OBJ_L3CACHE) caches[3].size += obj->attr->cache.size;
        if (obj->type == HWLOC_OBJ_L2CACHE) caches[2].size += obj->attr->cache.size;
        if (obj->type == HWLOC_OBJ_L1CACHE) caches[1].size += obj->attr->cache.size;
      }

      for (size_t i = 1; i < caches.size() + 1; i++){
        std::stringstream ss;
        ss << "_MKN_OMPI_HWLOC_L" << i << "_SIZE_=" << caches[i].size;
        a.add_def(ss.str());
        ss.str(std::string());
        ss << "_MKN_OMPI_HWLOC_L" << i << "_CL_SIZE_=" << caches[i].line_size;
        a.add_def(ss.str());
      }
    } catch (const mkn::kul::Exception& e) {
      KERR << e.stack();
    } catch (const std::exception& e) {
      KERR << e.what();
    } catch (...) {
      KERR << "UNKNOWN ERROR CAUGHT";
    }
  }
};
}  // namespace ompi
}  // namespace mkn

extern "C" KUL_PUBLISH maiken::Module* maiken_module_construct() {
  return new mkn::ompi::hwlocModule;
}

extern "C" KUL_PUBLISH void maiken_module_destruct(maiken::Module* p) {
  delete p;
}
