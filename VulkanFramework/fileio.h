#pragma once

#include <string>
#include <vector>
#include <fstream>

#include "assert.h"

inline std::vector<char> read_binary_file(std::string const& file_name)
{
  std::ifstream file(file_name, std::ios::binary);
  if (file.fail())
  {
    assert(("Could not open \"" + file_name + "\" file!").c_str(), "Vulkan", Assert::Error);
    return std::vector<char>();
  }

  std::streampos begin, end;
  begin = file.tellg();
  file.seekg(0, std::ios::end);
  end = file.tellg();

  std::vector<char> result(static_cast<size_t>(end - begin));
  file.seekg(0, std::ios::beg);
  file.read(&result[0], end - begin);
  file.close();

  return result;
}