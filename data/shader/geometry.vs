#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coords;

out vec3 v2f_pos;
out vec2 v2f_tex_coords;
out vec3 v2f_normal;

uniform bool invert_normal;//判断是否反转法向量，用于“盒内”的渲染

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;//物体坐标变换

void main()
{
    vec4 view_pos = view * model * vec4(pos, 1.0);
    v2f_pos = view_pos.xyz; 
    v2f_tex_coords = tex_coords;
    
    mat3 normal_matrix = transpose(inverse(mat3(view * model)));//法线矩阵
    v2f_normal = normal_matrix * (invert_normal ? -normal : normal);
    
    gl_Position = projection * view_pos;
}