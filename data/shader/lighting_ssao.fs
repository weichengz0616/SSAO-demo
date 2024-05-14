#version 330 core
// 每个片段/像素计算光照,在屏幕空间中
out vec4 frag_color;

in vec2 v2f_tex_coords;

//gbuffer
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
//模糊后的ssao纹理
uniform sampler2D ssao;

struct Light {
    //光源状态
    vec3 position;
    vec3 color;
    
    //衰减系数（线性和二次项）
    float linear;
    float quad;
};
uniform Light light;

void main()
{             
    // 直接从纹理计算几何信息
    vec3 frag_pos = texture(g_position, v2f_tex_coords).rgb;
    vec3 normal = texture(g_normal, v2f_tex_coords).rgb;
    vec3 albedo = texture(g_albedo, v2f_tex_coords).rgb;
    // SSAO作用于此
    float occlusion = texture(ssao, v2f_tex_coords).r;
    

    // 注意全程计算都是在观察空间中的
    // 布林冯模型计算光照
    // ambient
    vec3 ambient = vec3(0.3 * albedo * occlusion);
    vec3 lighting  = ambient; 
    vec3 view_dir  = normalize(-frag_pos);//观察空间
    // diffuse
    vec3 light_dir = normalize(light.position - frag_pos);
    vec3 diffuse = max(dot(normal, light_dir), 0.0) * albedo * light.color;
    // specular
    vec3 half_dir = normalize(light_dir + view_dir);  
    float spec = pow(max(dot(normal, half_dir), 0.0), 8.0);
    vec3 specular = light.color * spec;
    // 衰减
    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quad * distance * distance);
    diffuse *= attenuation;
    specular *= attenuation;
    
    lighting += diffuse + specular;

    frag_color = vec4(lighting, 1.0);
}
