#version 330 core
//计算SSAO纹理
//输出是每个像素/片段的遮蔽因子
out float frag_color;
//out vec4 frag_color;

in vec2 v2f_tex_coords;

//gbuffer读取
uniform sampler2D g_position;
uniform sampler2D g_normal;
//随机旋转纹理，需要缩放
uniform sampler2D noise_tex;

uniform vec3 samples[64];

// SSAO参数
int kernel_num = 64;
float radius = 0.5;
float bias = 0.025;

// 窗口大小和噪声纹理大小不一致，缩放至一致
// 800x800屏幕大小 4x4随即旋转噪声大小
const vec2 noise_scale = vec2(800.0/4.0, 800.0/4.0); 

uniform mat4 projection;

void main()
{
    // 直接从纹理中读取几何信息
    vec3 frag_pos = texture(g_position, v2f_tex_coords).xyz;
    vec3 normal = normalize(texture(g_normal, v2f_tex_coords).rgb);
    vec3 noise = normalize(texture(noise_tex, v2f_tex_coords * noise_scale).xyz);

    // TBN矩阵
    vec3 tangent = normalize(noise - normal * dot(noise, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // 计算遮蔽因子
    float occlusion = 0.0;
    for(int i = 0; i < kernel_num; i++)
    {
        // 计算样本位置，变换到观察空间
        vec3 sample_pos = TBN * samples[i];
        sample_pos = frag_pos + sample_pos * radius; //方向乘上半径 => 生成样本
        
        // 变换到屏幕空间
        vec4 offset = vec4(sample_pos, 1.0);
        offset = projection * offset; //先投影至裁剪空间
        offset.xyz /= offset.w; //透视
        offset.xyz = offset.xyz * 0.5 + 0.5; //变换值域[0,1]
        
        // 计算样本深度
        float sample_depth = texture(g_position, offset.xy).z; 
        
        // 范围检查
        // 当检测一个靠近表面边缘的片段时,它将会考虑测试表面之下的表面的深度值,这些值将会错误计算遮蔽因子
        // 只当被测深度值在取样半径内时影响遮蔽因子,光滑插值
        float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));

        // 样本的深度值是否大于当前片段存储的深度值
        occlusion += (sample_depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;           
    }
    // 直接利用该结果可以缩放环境光照量
    occlusion = 1.0 - (occlusion / kernel_num);
    
    frag_color = occlusion;
    //frag_color = vec4(vec3(occlusion),1.0);//用于展示SSAO纹理
}
