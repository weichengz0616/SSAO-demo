#version 330 core
out vec4 frag_color;

in vec2 v2f_tex_coords;

//gbuffer
uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
//模糊后的ssao纹理
uniform sampler2D ssao;

struct Light {
    vec3 position;
    vec3 color;
    
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
    // 这里关掉了SSAO
    float occlusion = 1.0;
    
    // 布林冯模型计算光照
    // ambient
    vec3 ambient = vec3(0.3 * albedo * occlusion);
    vec3 lighting  = ambient; 
    vec3 view_dir  = normalize(-frag_pos);
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
