#include <iostream>
#include <fstream>
#include <rumur/Model.h>
#include <rumur/output.h>
#include "resources.h"
#include <string>
#include <utility>
#include <vector>

namespace rumur {

// Whether a rule is a standard state transition rule.
int output_checker(const std::string &path, const Model &model,
  const OutputOptions &options) {

  std::ofstream out(path);
  if (!out)
    return -1;

  if (!options.debug)
    out << "#define NDEBUG 1\n\n";

  out

    // #includes
    << std::string((const char*)resources_includes_c, (size_t)resources_includes_c_len)
    << "\n"

    << "enum { SET_CAPACITY = " << options.set_capacity << "ul };\n\n"
    << "enum { SET_EXPAND_THRESHOLD = " << options.set_expand_threshold << " };\n\n"

    // xxHash source
    << std::string((const char*)resources_xxhash_h, (size_t)resources_xxhash_h_len)
    << "\n"

    // Settings that are used in header.c
    << "enum { THREADS = " << options.threads << "ul };\n\n"
    << "enum { STATE_SIZE_BITS = " << model.size_bits() << "ul };\n\n"

    // Static boiler plate code
    << std::string((const char*)resources_header_c, (size_t)resources_header_c_len)
    << "\n"

    // the model itself
    << model;

  return 0;

}

}
