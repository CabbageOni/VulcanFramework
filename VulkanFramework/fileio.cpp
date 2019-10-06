#include <string>
#include <vector>
#include <fstream>

#include "fileio.h"

#include "assert.h"
#define STB_IMAGE_IMPLEMENTATION
#include "resources\stbi\stb_image.h"
#undef assert

std::vector<char> read_binary_file(std::string const& file_name)
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

std::vector<char> read_image(std::string const& filename, int requested_components, int* width, int* height, int* components, int* data_size)
{
  std::vector<char> file_data = read_binary_file(filename);
  if (file_data.size() == 0)
  {
    assert("Could not read image data!", "Vulkan", Assert::Error);
    return std::vector<char>();
  }

  int tmp_width = 0, tmp_height = 0, tmp_components = 0;
  unsigned char* image_data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(&file_data[0]), static_cast<int>(file_data.size()), &tmp_width, &tmp_height, &tmp_components, requested_components);
  if ((image_data == nullptr) || (tmp_width <= 0) || (tmp_height <= 0) || (tmp_components <= 0))
  {
    assert("Could not read image data!", "Vulkan", Assert::Error);
    return std::vector<char>();
  }

  int size = (tmp_width) * (tmp_height) * (requested_components <= 0 ? tmp_components : requested_components);
  if (data_size)
    *data_size = size;
  if (width)
    *width = tmp_width;
  if (height)
    *height = tmp_height;
  if (components)
    *components = tmp_components;

  std::vector<char> output(size);
  memcpy(output.data(), image_data, size);

  stbi_image_free(image_data);
  return output;
}