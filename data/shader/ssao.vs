#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex_coords;

out vec2 v2f_tex_coords;

void main()
{
    v2f_tex_coords = tex_coords;
    gl_Position = vec4(pos, 1.0);
}