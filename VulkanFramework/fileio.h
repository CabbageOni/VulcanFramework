#pragma once

#include <string>
#include <vector>

std::vector<char> read_binary_file(std::string const& file_name);
std::vector<char> read_image(std::string const& filename, int requested_components, int* width, int* height, int* components, int* data_size);