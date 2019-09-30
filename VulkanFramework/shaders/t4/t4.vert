#version 450

layout(location = 0) in vec4 input_position;
layout(location = 1) in vec4 input_color;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec4 to_frag_color;

void main()
{
    gl_Position = input_position;
    to_frag_color = input_color;
}
