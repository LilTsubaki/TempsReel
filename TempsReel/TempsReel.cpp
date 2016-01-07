// TempsReel.cpp : définit le point d'entrée pour l'application console.
//

#include "stdafx.h"


#include <iostream>
#include <fstream>
#include <sstream>

#include <glew.h>

#include <glfw3.h>
#include <GL/gl.h>
#include <glm\vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "Maillage.h"

void render(GLFWwindow*);
void init();


#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

// This function is called on any openGL API error
void debug(GLenum, // source
	GLenum, // type
	GLuint, // id
	GLenum, // severity
	GLsizei, // length
	const GLchar *message,
	const void *) // userParam
{
	std::cout << "DEBUG: " << message << std::endl;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cerr << "Could not init glfw" << std::endl;
		return -1;
	}

	// This is a debug context, this is slow, but debugs, which is interesting
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		std::cerr << "Could not init window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cerr << "Could not init GLEW" << std::endl;
		std::cerr << glewGetErrorString(err) << std::endl;
		glfwTerminate();
		return -1;
	}

	// Now that the context is initialised, print some informations
	glInfo(GL_VENDOR);
	glInfo(GL_RENDERER);
	glInfo(GL_VERSION);
	glInfo(GL_SHADING_LANGUAGE_VERSION);

	// And enable debug
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	glDebugMessageCallback((GLDEBUGPROC)debug, nullptr);

	// This is our openGL init function which creates ressources
	init();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		render(window);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

// Build a shader from a string
GLuint buildShader(GLenum const shaderType, std::string const src)
{
	GLuint shader = glCreateShader(shaderType);

	const char* ptr = src.c_str();
	GLint length = src.length();

	glShaderSource(shader, 1, &ptr, &length);

	glCompileShader(shader);

	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (!res)
	{
		std::cerr << "shader compilation error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetShaderInfoLog(shader, 1000, &readSize, message);
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate();
		exit(-1);
	}

	return shader;
}

// read a file content into a string
std::string fileGetContents(const std::string path)
{
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();

	return buffer.str();
}

// build a program with a vertex shader and a fragment shader
GLuint buildProgram(const std::string vertexFile, const std::string fragmentFile)
{
	auto vshader = buildShader(GL_VERTEX_SHADER, fileGetContents(vertexFile));
	auto fshader = buildShader(GL_FRAGMENT_SHADER, fileGetContents(fragmentFile));

	GLuint program = glCreateProgram();

	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	glLinkProgram(program);

	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (!res)
	{
		std::cerr << "program link error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetProgramInfoLog(program, 1000, &readSize, message);
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate();
		exit(-1);
	}

	return program;
}



/****************************************************************
******* INTERESTING STUFFS HERE ********************************
***************************************************************/
static float cpt1 = 0;
static float cpt2 = 0;
static float cpt3 = 0;

static float tx = -0.5;
//static float datas[] = {-0.5,-0.5,0, 0.5,-0.5,0, 0.5,0.5,0, -0.5,0.5,0};
static GLuint buffer; 
static glm::mat4 projectionMatrix;
static glm::mat4 viewMatrix;
static glm::mat4 luxMatrix;
static int cptCamera = 0;
static int nbTriangles;

// Store the global state of your program
struct
{
	GLuint program; // a shader
	GLuint vao; // a vertex array object
	GLuint depthTexture;
	GLuint fbo;
} gs;


void init()
{
	glGenTextures(1, &gs.depthTexture);
	glBindTexture(GL_TEXTURE_2D, gs.depthTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 800, 800);

	glGenFramebuffers(1, &gs.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gs.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gs.depthTexture, 0);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gs.depthTexture);

	// Build our program and an empty VAO
	gs.program = buildProgram("basic.vsl", "basic.fsl");

	Maillage m;
	glm::vec3 min;
	glm::vec3 max;

	m.geometry(glm::vec3(0.f, 0.f, 0.f), "C:/Users/etu/Desktop/monkey.obj", min, max);

	std::vector<float> points = m.getAllPoints();
	nbTriangles = points.size();
	
	std::vector<float> norms = m.getallNormals();

	std::vector<float> all;
	all.reserve(points.size() * 2);

	for (int i = 0; i < points.size() ; i+=3)
	{
		all.push_back(points.at(i));
		all.push_back(points.at(i+1));
		all.push_back(points.at(i+2));
		all.push_back(norms.at(i));
		all.push_back(norms.at(i+1));
		all.push_back(norms.at(i+2));
	}

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	glBufferData(GL_ARRAY_BUFFER, all.size() * 4, all.data(), GL_STATIC_DRAW);


	glCreateVertexArrays(1, &gs.vao);
	glBindVertexArray(gs.vao);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	
	glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 6*4, 0);
	glVertexAttribPointer(13, 3, GL_FLOAT, GL_FALSE, 6 * 4, (void*)(3*4));


	glEnableVertexArrayAttrib(gs.vao, 12);
	glEnableVertexArrayAttrib(gs.vao, 13);
	glBindVertexArray(0);

	
}


void pass(GLuint buffer, int width, int height, glm::mat4 viewMatrix)
{
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	glViewport(0, 0, width, height);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(gs.vao);
	glUseProgram(gs.program);
	glEnable(GL_DEPTH_TEST);

	projectionMatrix = glm::perspective(45.0f, 640.0f / 480.0f, 1.0f, 200.0f);

	glm::mat4 MVP = projectionMatrix * viewMatrix;


	glProgramUniformMatrix4fv(gs.program, 15, 1, GL_FALSE, &MVP[0][0]);


	glProgramUniform1f(gs.program, 3, 1); //fmod(cpt1 += 0.01, 1));
	glProgramUniform1f(gs.program, 4, 1); //fmod(cpt2 += 0.05, 1));
	glProgramUniform1f(gs.program, 5, 1); // fmod(cpt3 += 0.02, 1));

	glProgramUniform3f(gs.program, 6, 0.5, -1, -2);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, nbTriangles);

	glViewport(0, 0, width, height);

	glUseProgram(0);
}


void render(GLFWwindow* window)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	luxMatrix = glm::lookAt(glm::vec3(0.5, -1, -2), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
	viewMatrix = glm::lookAt(glm::vec3(sin(cptCamera++ / 100.0) * 5, -5, cos(cptCamera / 100.0) * 5), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));

	projectionMatrix = glm::perspective(45.0f, 640.0f / 480.0f, 1.0f, 200.0f);
	glm::mat4 MVP = projectionMatrix * luxMatrix;
	glProgramUniformMatrix4fv(gs.program, 16, 1, GL_FALSE, &MVP[0][0]);



	
	

	pass(gs.depthTexture, 800, 800, luxMatrix);

	pass(0, width, height, viewMatrix);


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


