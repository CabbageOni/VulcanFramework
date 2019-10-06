#version 450

layout(set=0, binding=0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 texture_coord;

layout(location = 0) out vec4 output_Color;

void main()
{
  output_Color = texture( u_Texture, texture_coord);
}