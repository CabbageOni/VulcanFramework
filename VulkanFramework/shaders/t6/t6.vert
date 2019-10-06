#version 450

layout(location = 0) in vec4 input_position;
layout(location = 1) in vec2 input_texture_coord;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec2 texture_coord;

void main()
{
    gl_Position = input_position;
    texture_coord = input_texture_coord;
}
