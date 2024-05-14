#version 330 core
//这个帧缓冲的纹理用于存储模糊上面的SSAO纹理的模糊结果,将直接用于光照计算阶段
out float frag_color;//最终结果
//out vec4 frag_color;

in vec2 v2f_tex_coords;

//读取的ssao纹理
uniform sampler2D ssao;

void main() 
{
    //ssao纹理单元的大小
    vec2 texel_size = 1.0 / vec2(textureSize(ssao, 0));

    float result = 0.0;
    for (int x = -2; x < 2; x++) 
    {
        for (int y = -2; y < 2; y++) 
        {
            //计算周围16个纹理单元
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            //纹理采样
            result += texture(ssao, v2f_tex_coords + offset).r;
        }
    }

    //通过平均操作进行模糊
    frag_color = result / (4.0 * 4.0);

    // float tmp = result / (4.0 * 4.0);
    // frag_color = vec4(vec3(tmp),1.0);//用于展示模糊后的SSAO纹理
}  