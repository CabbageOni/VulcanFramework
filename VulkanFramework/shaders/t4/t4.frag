#version 450

layout(location = 0) in vec4 to_frag_Color;

layout(location = 0) out vec4 output_Color;

void main()
{
  output_Color = to_frag_Color;
}