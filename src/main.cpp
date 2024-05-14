#include "gl_env.h"
#include "shader.h"
#include "mesh.h"
#include "model.h"

#include <cstdlib>
#include <cstdio>
#include <config.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stb_image.h>

#ifndef M_PI
#define M_PI (3.1415926535897932)
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <random>

//using namespace std;

//�Ƿ��SSAO
bool SSAO = true;


//���ڴ�С
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;


//�ص�����
static void error_callback(int error, const char* description);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


//��Ⱦ���Σ�����gbuffer
//��Ⱦ�����壺������Ⱦһ�������䡱
void renderQuad();
void renderCube();


//lerp
float my_lerp(float a, float b, float f);


//��Դ��Ϣ
glm::vec3 light_pos = glm::vec3(2.0, 4.0, -2.0);
//glm::vec3 light_color = glm::vec3(0.2, 0.2, 0.7);
glm::vec3 light_color = glm::vec3(1.0, 1.0, 1.0);
//����˥������
const float linear = 0.09f;
const float quadratic = 0.032f;


//world ��׼ֵ
glm::vec3 world_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
//���
struct Camera
{
	glm::vec3 pos;
	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;

	Camera()
	{
		pos = glm::vec3(0.0f, 0.0f, 0.0f);
		front = glm::vec3(0.0f, 0.0f, 0.0f);
		right = glm::vec3(0.0f, 0.0f, 0.0f);
		up = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	Camera(glm::vec3 pos_, glm::vec3 front_, glm::vec3 right_, glm::vec3 up_)
	{
		pos = pos_;
		front = front_;
		right = right_;
		up = up_;
	}
	Camera(glm::vec3 pos_, glm::vec3 front_)
	{
		pos = pos_;
		front = front_;
		right = glm::normalize(glm::cross(front, world_up));
		up = glm::normalize(glm::cross(right, front));
	}

};
//camera��ʼ״̬
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;


//timing
float delta_time = 0.0f;
float last_frame = 0.0f;



int main(int argc, char *argv[]) 
{
	//================================ ��ʼ������ ================================//
	GLFWwindow *window;
	
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__ // for macos
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL output", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	//callback
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	if (glewInit() != GLEW_OK)
		exit(EXIT_FAILURE);

	glEnable(GL_DEPTH_TEST);
	//================================ ��ʼ������ ================================//






	//================================ shader ================================//
	//���ڷ��ͼ�����Ϣ��gbuffer��֡����
	Shader shader_geometry(DATA_DIR"/shader/geometry.vs", DATA_DIR"/shader/geometry.fs");
	//�������յĹ��ռ��㲢��ʾ����Ļ
	Shader shader_lighting(DATA_DIR"/shader/ssao.vs", DATA_DIR"/shader/lighting_ssao.fs");
	//��������SSAO����
	Shader shader_ssao(DATA_DIR"/shader/ssao.vs", DATA_DIR"/shader/ssao.fs");
	//����SSAOģ������
	Shader shader_blur(DATA_DIR"/shader/ssao.vs", DATA_DIR"/shader/blur.fs");
	//���ڶԱ�SSAOЧ������SSAO�Ĺ�����Ⱦ
	Shader shader_lighting_nossao(DATA_DIR"/shader/ssao.vs", DATA_DIR"/shader/lighting_nossao.fs");
	//================================ shader ================================//







	//================================ ����ģ�� ================================//
	//ע�������õ�assimp���ǲ������ģ�ֻ�ܼ��� FBX ģ���ļ�
	Model my_model(DATA_DIR"/fbx/dragon.fbx");
	//================================ ����ģ�� ================================//






	//================================ ����gbuffer ================================//
	unsigned int g_buffer;
	glGenFramebuffers(1, &g_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	unsigned int g_position, g_normal, g_albedo;//��Ҫ������Ϣ

	// position
	glGenTextures(1, &g_position);
	glBindTexture(GL_TEXTURE_2D, g_position);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_position, 0);

	// normal
	glGenTextures(1, &g_normal);
	glBindTexture(GL_TEXTURE_2D, g_normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normal, 0);

	// color/albedo + specular (RGB + A)
	glGenTextures(1, &g_albedo);
	glBindTexture(GL_TEXTURE_2D, g_albedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_albedo, 0);
	
	// MRT ����3����ɫ����
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// depth ����Ⱦ����
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// ���֡����������
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "gbuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//================================ ����gbuffer ================================//






	//================================ ����SSAO���֡���� ================================//
	unsigned int ssao_fbo, blur_fbo;
	glGenFramebuffers(1, &ssao_fbo);
	glGenFramebuffers(1, &blur_fbo);

	unsigned int ssao_tex;
	unsigned int blur_tex;//�������

	// SSAO
	// ���֡������������ڴ洢��Ļ�ռ���ÿ��Ƭ�ε��ڱ�����,��Ļ�ռ伸����Ϣ��gbuffer�ж�ȡ
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
	glGenTextures(1, &ssao_tex);
	glBindTexture(GL_TEXTURE_2D, ssao_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);//�ڱ������Ǹ�float,ֻʹ����һ������
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_tex, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Framebuffer not complete!" << std::endl;

	// blur
	// ����һ���⻬�Ļ����ڱν����Ҫ ģ�� �����ڱ�����
	// ���֡������������ڴ洢ģ�������SSAO�����ģ�����,��ֱ�����ڹ��ռ���׶�
	glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo);
	glGenTextures(1, &blur_tex);
	glBindTexture(GL_TEXTURE_2D, blur_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_tex, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//================================ ����SSAO���֡���� ================================//
	





	//================================ ���ɲ���kernel ================================//
	//���ȷֲ��������
	std::uniform_real_distribution<GLfloat> random_float(0.0, 1.0);
	std::default_random_engine generator;

	std::vector<glm::vec3> ssao_kernel;

	//64������
	for (unsigned int i = 0; i < 64; i++)
	{
		//���������߿ռ���
		//xy��[-1,1],,,,z��[0,1] => �������
		glm::vec3 sample(random_float(generator) * 2.0 - 1.0, random_float(generator) * 2.0 - 1.0, random_float(generator));
		
		sample = glm::normalize(sample);
		sample *= random_float(generator);

		//ʹ��lerp��������������Ƭ�θ���
		float scale = float(i) / 64.0f;
		scale = my_lerp(0.1f, 1.0f, scale * scale);

		sample *= scale;
		ssao_kernel.push_back(sample);
	}
	//================================ ���ɲ���kernel ================================//
	
	
	
	


	//================================ ���������ת ================================//
	//����һ��4x4�������߿ռ�ƽ�淨�ߵ������ת��������,����  ����  ������Ļ�ռ�
	std::vector<glm::vec3> ssao_noise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(random_float(generator) * 2.0 - 1.0, random_float(generator) * 2.0 - 1.0, 0.0f);
		ssao_noise.push_back(noise);
	}

	//4x4����,�洢�����ת����
	unsigned int noise_tex; 
	glGenTextures(1, &noise_tex);
	glBindTexture(GL_TEXTURE_2D, noise_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssao_noise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//================================ ���������ת ================================//

	
	



	//================================ ����shader������ ================================//
	shader_lighting.use();
	shader_lighting.setInt("g_position", 0);
	shader_lighting.setInt("g_normal", 1);
	shader_lighting.setInt("g_albedo", 2);
	shader_lighting.setInt("ssao", 3);

	shader_lighting_nossao.use(); 
	shader_lighting_nossao.setInt("g_position", 0);
	shader_lighting_nossao.setInt("g_normal", 1);
	shader_lighting_nossao.setInt("g_albedo", 2);
	shader_lighting_nossao.setInt("ssao", 3);

	shader_ssao.use();
	shader_ssao.setInt("g_position", 0);
	shader_ssao.setInt("g_normal", 1);
	shader_ssao.setInt("noise_tex", 2);

	shader_blur.use();
	shader_blur.setInt("ssao", 0);
	//================================ ����shader������ ================================//

	
	




	//********************************************************************** //
	//***********************      ��ѭ��      *******************************//
	//********************************************************************** //
	while (!glfwWindowShouldClose(window)) 
	{
		//================================ �������� ================================//
		//timing
		float current_frame = (float)glfwGetTime();
		delta_time = current_frame - last_frame;
		last_frame = current_frame;

		//���
		glm::vec3 current_front = camera.front;
		glm::vec3 current_pos = camera.pos;
		

		float ratio;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float) height;
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//================================ �������� ================================//


		


		//================================ һ����������Ϣ������gbuffer ================================//
		//glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 projection = glm::perspective(glm::radians(fov), ratio, 0.1f, 50.0f);
		glm::mat4 view = glm::lookAt(current_pos, current_pos + current_front, world_up);
		glm::mat4 model = glm::mat4(1.0f);
		shader_geometry.use();
		shader_geometry.setMat4("projection", projection);
		shader_geometry.setMat4("view", view);


		// ��Ⱦ���䣨cube��
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, 7.0f, 0.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		shader_geometry.setMat4("model", model);
		// ���λ�ں����ڲ�����ת������
		shader_geometry.setInt("invert_normal", 1);
		renderCube();//��Ⱦ��gbuffer
		shader_geometry.setInt("invert_normal", 0);


		// ��Ⱦģ��
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		shader_geometry.setMat4("model", model);
		my_model.Draw(shader_geometry);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//================================ һ����������Ϣ������gbuffer ================================//


		


		//================================ ��������SSAO֡���� ================================//
		glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
		glClear(GL_COLOR_BUFFER_BIT);
		shader_ssao.use();
		
		for (unsigned int i = 0; i < 64; i++)
			shader_ssao.setVec3("samples[" + std::to_string(i) + "]", ssao_kernel[i]);

		shader_ssao.setMat4("projection", projection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_position);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_normal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noise_tex);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//================================ ��������SSAO֡���� ================================//






		//================================ ����ģ������ ================================//
		glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo);
		glClear(GL_COLOR_BUFFER_BIT);

		shader_blur.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssao_tex);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//================================ ����ģ������ ================================//






		//================================ �ġ�������Ⱦ ================================//
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//���ù�Դ
		glm::vec3 light_pos_view = glm::vec3(view * glm::vec4(light_pos, 1.0));
		//glDisable(GL_DEPTH_TEST);
		if (SSAO)//ʹ��SSAO
		{
			shader_lighting.use();

			shader_lighting.setVec3("light.position", light_pos_view);
			shader_lighting.setVec3("light.color", light_color);
			shader_lighting.setFloat("light.linear", linear);
			shader_lighting.setFloat("light.quad", quadratic);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, g_position);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, g_normal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, g_albedo);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, blur_tex);//SSAO
			renderQuad();
		}
		else//��ʹ��SSAO
		{
			shader_lighting_nossao.use();

			shader_lighting_nossao.setVec3("light.position", light_pos_view);
			shader_lighting_nossao.setVec3("light.color", light_color);
			shader_lighting_nossao.setFloat("light.linear", linear);
			shader_lighting_nossao.setFloat("light.quad", quadratic);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, g_position);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, g_normal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, g_albedo);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, blur_tex);//�����SSAO��shader_lighting_nossao�в�δʹ��
			renderQuad();
		}
		//================================ ������Ⱦ ================================//
		
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}





unsigned int cube_vao = 0;
unsigned int cube_vbo = 0;
void renderCube()
{
	if (cube_vao == 0)//����0˵���ǵ�һ��
	{
		float vertices[] = 
		{
			// ����
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,          
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 
			// ǰ��
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, 
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 
			// ����
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, 
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
			// ����
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,      
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,    
			// ����
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, 
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, 
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 
			// ����
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,    
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f        
		};

		glGenVertexArrays(1, &cube_vao);
		glGenBuffers(1, &cube_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindVertexArray(cube_vao);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cube_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}



unsigned int quad_vao = 0;
unsigned int quad_vbo;
void renderQuad()
{
	if (quad_vao == 0)//����0˵���ǵ�һ��
	{
		float vertices[] = 
		{
			//λ��				//��������
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
	
		glGenVertexArrays(1, &quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindVertexArray(quad_vao);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}



float my_lerp(float a, float b, float f)
{
	return a + f * (b - a);
}



static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	//camera control
	float camera_speed = 0.6f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.pos += camera_speed * camera.up;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.pos -= camera_speed * camera.up;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.pos -= camera_speed * camera.right;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.pos += camera_speed * camera.right;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		SSAO = 1 - SSAO;
}
static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//control mouse
	static bool first_mouse = true;
	static float last_x = 800.0f / 2.0;
	static float last_y = 600.0 / 2.0;

	if (first_mouse)
	{
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}

	float xoffset = xpos - last_x;
	float yoffset = last_y - ypos;
	//printf("%f\n", yoffset);
	last_x = xpos;
	last_y = ypos;

	float sensitivity = 0.1;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	/*glm::quat yaw_quat = glm::angleAxis(glm::radians(yaw), world_up);
	glm::quat pitch_quat = glm::angleAxis(glm::radians(pitch), glm::normalize(glm::cross(world_front, world_up)));
	glm::quat rot_quat = yaw_quat * pitch_quat;*/

	camera.front = front;
	camera.right = glm::normalize(glm::cross(camera.front, world_up));
	camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (fov >= 1.0f && fov <= 45.0f)
		fov -= yoffset;
	if (fov <= 1.0f)
		fov = 1.0f;
	if (fov >= 45.0f)
		fov = 45.0f;
}