#version 330 core
// gbuffer有3个缓冲附件
layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 albedo;

in vec3 v2f_pos;
in vec2 v2f_tex_coords;
in vec3 v2f_normal;

void main()
{   
    //gbuffer中的几何信息
    position = v2f_pos;
    normal = normalize(v2f_normal);
    
    //为了显示SSAO效果，这里直接使用白模，未对模型进行贴图
    albedo = vec3(0.95);
}