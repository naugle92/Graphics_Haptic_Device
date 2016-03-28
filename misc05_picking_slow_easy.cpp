// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <stack>   
#include <sstream>
// Include GLEW
#include <GL/glew.h>
#include <GL/GLU.h>
// Include GLFW
#include <glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;
// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#define pi 3.14159265
#define GLFW_KEY_B   66
#define GLFW_KEY_C   67
#define GLFW_MOD_SHIFT   0x0001

const int window_width = 1024, window_height = 768;

typedef struct Vertex {
	float Position[4];
	float Color[4];
	float Normal[3];
	void SetPosition(float *coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetColor(float *color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
	void SetNormal(float *coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
};

// function prototypes
int initWindow(void);
void initOpenGL(void);
void loadObject(char*, glm::vec4, Vertex * &, GLushort* &, int);
void createVAOs(Vertex[], GLushort[], int);
void createObjects(void);
void move_camera(int, int);
void pickObject(void);
void renderScene(void);
void cleanup(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);

// GLOBAL VARIABLES

bool bpressed;
bool cpressed;
bool tpressed;
bool onepressed;
bool twopressed;
bool ppressed;

float camerax;
float cameray;
float cameraz;
float thetax;
float thetay;

float baseX, baseZ;
float top_rotation;
float arm1_rotate;
float joint_rotation;
float arm2_rotation;
float button_rotation;
float pen_rotation;

Vertex Base_Verts[500];
Vertex* Top_Verts;

GLFWwindow* window;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

GLuint gPickedIndex = -1;
std::string gMessage;

GLuint programID;
GLuint pickingProgramID;

const GLuint NumObjects = 15;	// ATTN: THIS NEEDS TO CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects] = { 0 };
GLuint VertexBufferId[NumObjects] = { 0 };
GLuint IndexBufferId[NumObjects] = { 0 };

size_t NumIndices[NumObjects] = { 0 };
size_t VertexBufferSize[NumObjects] = { 0 };
size_t IndexBufferSize[NumObjects] = { 0 };

GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ModelMatrixID2;
GLuint ViewMatrixID;
GLuint ViewMatrixID2;
GLuint ProjMatrixID;
GLuint ProjMatrixID2;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint LightID;
GLuint LightID2;
GLuint normalbuffer;

GLint gX = 0.0;
GLint gZ = 0.0;

// animation control
bool animation = false;
GLfloat phi = 0.0;

void loadObject(char* file, glm::vec4 color, Vertex * &out_Vertices, GLushort* &out_Indices, int ObjectId)
{
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(file, vertices, normals);

	std::vector<GLushort> indices;
	std::vector<GLushort> indicesgrid;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, normals, indices, indexed_vertices, indexed_normals);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();

	/*GLfloat material_diffuse[] = { 1, 0, 0, 1 };
	GLfloat material_specular[] = { 1, 1, 1, 1 };
	GLfloat material_shininess[] = { 100 };
	glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, material_shininess);
	*/
	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);
		out_Vertices[i].SetColor(&color[0]);
	}
	out_Indices = new GLushort[idxCount];
	for (int i = 0; i < idxCount; i++) {
		out_Indices[i] = indices[i];
	}

	// set global variables!!
	NumIndices[ObjectId] = idxCount;
	VertexBufferSize[ObjectId] = sizeof(out_Vertices[0]) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;


	//do something with the normals
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
}


void createObjects(void)
{
	//-- COORDINATE AXES --//
	Vertex CoordVerts[] =
	{
		{ { 0.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 5.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } },
		{ { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } }
	};
	Vertex grid[] = {
		{ { -5.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0, -4.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0, -4.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0, -3.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0, -3.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0, -2.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0, -2.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0, -1.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0, -1.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  0.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  0.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  1.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  1.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  2.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  2.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  3.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  3.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  4.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  4.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },

		{ { -5.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -5.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -4.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -4.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -3.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -3.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -2.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -2.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -1.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ { -1.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  0.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  0.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  1.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  1.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  2.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  2.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  3.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  3.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  4.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  4.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0, -5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
		{ {  5.0, 0.0,  5.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } }


	};

	VertexBufferSize[0] = sizeof(CoordVerts);	// ATTN: this needs to be done for each hand-made object with the ObjectID (subscript)
	VertexBufferSize[1] = sizeof(grid);
	createVAOs(CoordVerts, NULL, 0);
	createVAOs(grid, NULL, 1);
	
	//-- GRID --//
	
	// ATTN: create your grid vertices here!
	
	//-- .OBJs --//

	// ATTN: load your models here
	Vertex* Verts;
	GLushort* Idcs;
	loadObject("models/base.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), Verts, Idcs, 2);
	createVAOs(Verts, Idcs, 2);

	loadObject("models/top.obj", glm::vec4(0.0, 1.0, 0.0, 1.0), Verts, Idcs, 3);
	createVAOs(Verts, Idcs, 3);

	loadObject("models/arm1.obj", glm::vec4(0.0, 0.0, 1.0, 1.0), Verts, Idcs, 4);
	createVAOs(Verts, Idcs, 4);

	loadObject("models/joint.obj", glm::vec4(1.0, 0.25, 0.75, 1.0), Verts, Idcs, 5);
	createVAOs(Verts, Idcs, 5);

	loadObject("models/arm2.obj", glm::vec4(0.0, 1.0, 1.0, 1.0), Verts, Idcs, 6);
	createVAOs(Verts, Idcs, 6);

	loadObject("models/button.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), Verts, Idcs, 7);
	createVAOs(Verts, Idcs, 7);

	loadObject("models/pen.obj", glm::vec4(1.0, 1.0, 0.0, 1.0), Verts, Idcs, 8);
	createVAOs(Verts, Idcs, 8);

	loadObject("models/base.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 9);
	createVAOs(Verts, Idcs, 9);

	loadObject("models/top.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 10);
	createVAOs(Verts, Idcs, 10);

	loadObject("models/arm1.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 11);
	createVAOs(Verts, Idcs, 11);

	loadObject("models/arm2.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 13);
	createVAOs(Verts, Idcs, 13);

	loadObject("models/pen.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 15);
	createVAOs(Verts, Idcs, 15);

}

void renderScene(void)
{
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!


	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);
	{
		//glm::vec3 lightPos = glm::vec3(camerax + 0.5, cameray, cameraz + 0.5);
		glm::vec3 lightPos = glm::vec3(5, 4, -5);
		glm::vec3 lightPos2 = glm::vec3(-5, 4, 5);
		glm::mat4x4 ModelMatrix = glm::mat4(1.0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(LightID2, lightPos2.x, lightPos2.y, lightPos2.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		// draw CoordAxes
		glBindVertexArray(VertexArrayId[0]);	
		glDrawArrays(GL_LINES, 0, 6);
			
		//draw grid
		glBindVertexArray(VertexArrayId[1]);
		glDrawArrays(GL_LINES, 0, 44);
		
		//draw base
		if (bpressed) { glBindVertexArray(VertexArrayId[9]); }
		else { glBindVertexArray(VertexArrayId[2]); }
		glm::mat4x4 Base_ModelMatrix = glm::mat4(1.0);
		Base_ModelMatrix = glm::translate(Base_ModelMatrix, glm::vec3(baseX, 0.4f, baseZ));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Base_ModelMatrix[0][0]);
		if (bpressed) { glDrawElements(GL_TRIANGLES, VertexBufferSize[9], GL_UNSIGNED_SHORT, 0); }
		else { glDrawElements(GL_TRIANGLES, VertexBufferSize[2], GL_UNSIGNED_SHORT, 0); }
		

		//draw top
		if (tpressed) { glBindVertexArray(VertexArrayId[10]); }
		else { glBindVertexArray(VertexArrayId[3]); }
		glm::mat4x4 Top_ModelMatrix = glm::mat4(1.0);
		Top_ModelMatrix = glm::translate(Base_ModelMatrix, glm::vec3(0.0f, 0.4f, 0.0f));
		Top_ModelMatrix = glm::translate(Top_ModelMatrix, glm::vec3(0.0f, 0.4f, 0.0f));
		Top_ModelMatrix = glm::rotate(Top_ModelMatrix, top_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Top_ModelMatrix[0][0]);
		if (tpressed) { glDrawElements(GL_TRIANGLES, VertexBufferSize[10], GL_UNSIGNED_SHORT, 0); }
		else { glDrawElements(GL_TRIANGLES, VertexBufferSize[3], GL_UNSIGNED_SHORT, 0); }

		//draw arm1
		if (onepressed) { glBindVertexArray(VertexArrayId[11]); }
		else { glBindVertexArray(VertexArrayId[4]); }
		glm::mat4x4 Arm1_ModelMatrix = glm::mat4(1.0);
		Arm1_ModelMatrix = glm::translate(Top_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Arm1_ModelMatrix = glm::rotate(Arm1_ModelMatrix, arm1_rotate, glm::vec3(0.0f, 0.0f, 1.0f));
		Arm1_ModelMatrix = glm::translate(Arm1_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Arm1_ModelMatrix[0][0]);
		if (onepressed) { glDrawElements(GL_TRIANGLES, VertexBufferSize[11], GL_UNSIGNED_SHORT, 0); }
		else { glDrawElements(GL_TRIANGLES, VertexBufferSize[4], GL_UNSIGNED_SHORT, 0); }
		

		//draw joint
		glBindVertexArray(VertexArrayId[5]);
		glm::mat4x4 Joint_ModelMatrix = glm::mat4(1.0);
		Joint_ModelMatrix = glm::translate(Arm1_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Joint_ModelMatrix = glm::translate(Joint_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		Joint_ModelMatrix = glm::rotate(Joint_ModelMatrix, joint_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		//Joint_ModelMatrix = glm::translate(Joint_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Joint_ModelMatrix[0][0]);
		glDrawElements(GL_TRIANGLES, VertexBufferSize[5], GL_UNSIGNED_SHORT, 0);


		//draw arm2
		if (twopressed) { glBindVertexArray(VertexArrayId[13]); }
		else { glBindVertexArray(VertexArrayId[6]); }
		glm::mat4x4 Arm2_ModelMatrix = glm::mat4(1.0);
		Arm2_ModelMatrix = glm::translate(Joint_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Arm2_ModelMatrix = glm::rotate(Arm2_ModelMatrix, joint_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		Arm2_ModelMatrix = glm::translate(Arm2_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Arm2_ModelMatrix[0][0]);
		if (twopressed) { glDrawElements(GL_TRIANGLES, VertexBufferSize[13], GL_UNSIGNED_SHORT, 0); }
		else { glDrawElements(GL_TRIANGLES, VertexBufferSize[6], GL_UNSIGNED_SHORT, 0); }

		
		//draw pen
		if (ppressed) { glBindVertexArray(VertexArrayId[15]); }
		else { glBindVertexArray(VertexArrayId[8]); }
		glm::mat4x4 Pen_ModelMatrix = glm::mat4(1.0);
		Pen_ModelMatrix = glm::translate(Arm2_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Pen_ModelMatrix = glm::translate(Pen_ModelMatrix, glm::vec3(0.0f, 0.6f, 0.0f));
		Pen_ModelMatrix = glm::rotate(Pen_ModelMatrix, arm2_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		Pen_ModelMatrix = glm::rotate(Pen_ModelMatrix, button_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		Pen_ModelMatrix = glm::translate(Pen_ModelMatrix, glm::vec3(0.0f, 0.2f, 0.0f));
		Pen_ModelMatrix = glm::rotate(Pen_ModelMatrix, pen_rotation, glm::vec3(0.0f, 2.0f, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Pen_ModelMatrix[0][0]);
		if (ppressed) { glDrawElements(GL_TRIANGLES, VertexBufferSize[15], GL_UNSIGNED_SHORT, 0); }
		else { glDrawElements(GL_TRIANGLES, VertexBufferSize[8], GL_UNSIGNED_SHORT, 0); }
		

		//draw button
		glBindVertexArray(VertexArrayId[7]);
		glm::mat4x4 Button_ModelMatrix = glm::mat4(1.0);
		float number = pi / 2.0;
		Button_ModelMatrix = glm::translate(Pen_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Button_ModelMatrix = glm::rotate(Button_ModelMatrix, number, glm::vec3(0.0f, 0.0f, 1.0f));
		Button_ModelMatrix = glm::translate(Button_ModelMatrix, glm::vec3(0.0f, 0.05f, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Button_ModelMatrix[0][0]);
		glDrawElements(GL_TRIANGLES, VertexBufferSize[7], GL_UNSIGNED_SHORT, 0);


		glBindVertexArray(0);

	}
	glUseProgram(0);
	// Draw GUI
	TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void pickObject(void)
{
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		
		// ATTN: DRAW YOUR PICKING SCENE HERE. REMEMBER TO SEND IN A DIFFERENT PICKING COLOR FOR EACH OBJECT BEFOREHAND
		//// draw Base
		//glBindVertexArray(X);	
		//	glUniform1f(pickingColorID, Y / 255.0f); // here we pass in the picking marker
		//	glDrawElements(Z);
		//// draw Top
		//glBindVertexArray(XX);	
		//	glUniform1f(pickingColorID, YY / 255.0f); // here we pass in the picking marker
		//	glDrawElements(ZZ);
		//glBindVertexArray(0);s


		//draw base
		glBindVertexArray(VertexArrayId[2]);
		
		glm::mat4x4 Base_ModelMatrix = glm::mat4(1.0);
		Base_ModelMatrix = glm::translate(Base_ModelMatrix, glm::vec3(baseX, 0.4f, baseZ));
		MVP = gProjectionMatrix * gViewMatrix * Base_ModelMatrix;
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform1f(pickingColorID, 1 / 255.0f); // here we pass in the picking marker
		glDrawElements(GL_TRIANGLES, VertexBufferSize[2], GL_UNSIGNED_SHORT, 0);
		
		//draw top
		glBindVertexArray(VertexArrayId[3]);
		glm::mat4x4 Top_ModelMatrix = glm::mat4(1.0);
		Top_ModelMatrix = glm::translate(Base_ModelMatrix, glm::vec3(0.0f, 0.4f, 0.0f));
		Top_ModelMatrix = glm::translate(Top_ModelMatrix, glm::vec3(0.0f, 0.4f, 0.0f));
		Top_ModelMatrix = glm::rotate(Top_ModelMatrix, top_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		MVP = gProjectionMatrix * gViewMatrix * Top_ModelMatrix;
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform1f(pickingColorID, 2 / 255.0f); // here we pass in the picking marker
		glDrawElements(GL_TRIANGLES, VertexBufferSize[3], GL_UNSIGNED_SHORT, 0);
		
		//draw arm1
		glBindVertexArray(VertexArrayId[4]);
		glm::mat4x4 Arm1_ModelMatrix = glm::mat4(1.0);
		Arm1_ModelMatrix = glm::translate(Top_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Arm1_ModelMatrix = glm::rotate(Arm1_ModelMatrix, arm1_rotate, glm::vec3(0.0f, 0.0f, 1.0f));
		Arm1_ModelMatrix = glm::translate(Arm1_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		MVP = gProjectionMatrix * gViewMatrix * Arm1_ModelMatrix;
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform1f(pickingColorID, 3 / 255.0f); // here we pass in the picking marker
		glDrawElements(GL_TRIANGLES, VertexBufferSize[4], GL_UNSIGNED_SHORT, 0);


		//draw joint
		glBindVertexArray(VertexArrayId[5]);
		glm::mat4x4 Joint_ModelMatrix = glm::mat4(1.0);
		Joint_ModelMatrix = glm::translate(Arm1_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Joint_ModelMatrix = glm::translate(Joint_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		Joint_ModelMatrix = glm::rotate(Joint_ModelMatrix, joint_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		MVP = gProjectionMatrix * gViewMatrix * Joint_ModelMatrix;
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform1f(pickingColorID, 4 / 255.0f); // here we pass in the picking marker
		glDrawElements(GL_TRIANGLES, VertexBufferSize[5], GL_UNSIGNED_SHORT, 0);


		//draw arm2
		glBindVertexArray(VertexArrayId[6]);
		glm::mat4x4 Arm2_ModelMatrix = glm::mat4(1.0);
		Arm2_ModelMatrix = glm::translate(Joint_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Arm2_ModelMatrix = glm::rotate(Arm2_ModelMatrix, joint_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		Arm2_ModelMatrix = glm::translate(Arm2_ModelMatrix, glm::vec3(0.0f, 1.0f, 0.0f));
		MVP = gProjectionMatrix * gViewMatrix * Arm2_ModelMatrix;
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform1f(pickingColorID, 5 / 255.0f); // here we pass in the picking marker
		glDrawElements(GL_TRIANGLES, VertexBufferSize[6], GL_UNSIGNED_SHORT, 0);



		//draw pen
		glBindVertexArray(VertexArrayId[8]);
		glm::mat4x4 Pen_ModelMatrix = glm::mat4(1.0);
		Pen_ModelMatrix = glm::translate(Arm2_ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		Pen_ModelMatrix = glm::translate(Pen_ModelMatrix, glm::vec3(0.0f, 0.6f, 0.0f));
		Pen_ModelMatrix = glm::rotate(Pen_ModelMatrix, arm2_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
		Pen_ModelMatrix = glm::rotate(Pen_ModelMatrix, button_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		Pen_ModelMatrix = glm::translate(Pen_ModelMatrix, glm::vec3(0.0f, 0.2f, 0.0f));
		Pen_ModelMatrix = glm::rotate(Pen_ModelMatrix, pen_rotation, glm::vec3(0.0f, 2.0f, 0.0f));
		MVP = gProjectionMatrix * gViewMatrix * Pen_ModelMatrix;
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniform1f(pickingColorID, 6 / 255.0f); // here we pass in the picking marker
		glDrawElements(GL_TRIANGLES, VertexBufferSize[8], GL_UNSIGNED_SHORT, 0);
		

	}
	glUseProgram(0);
	// Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.s
	glFlush();
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	//printf("data = %f\n", data);


	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);
	
	if (gPickedIndex == 255){ // Full white, must be the background !
		gMessage = "background";
	}
	else {
		std::ostringstream oss;

		if (gPickedIndex == 1) {
			bpressed = true;
			cpressed = false;
			tpressed = false;
			onepressed = false;
			twopressed = false;
			ppressed = false;
			oss << "Base";
		}
		
		if (gPickedIndex == 2) {
			bpressed = false;
			cpressed = false;
			tpressed = true;
			onepressed = false;
			twopressed = false;
			ppressed = false;
			oss << "Top";
			
		}
		if (gPickedIndex == 3) {
			bpressed = false;
			cpressed = false;
			tpressed = false;
			onepressed = true;
			twopressed = false;
			ppressed = false;
			oss << "Arm 1";
		}
		if (gPickedIndex == 5) {
			bpressed = false;
			cpressed = false;
			tpressed = false;
			onepressed = false;
			twopressed = true;
			ppressed = false;
			oss << "Arm 2";
		}
		if (gPickedIndex == 6) {
			bpressed = false;
			cpressed = false;
			tpressed = false;
			onepressed = false;
			twopressed = false;
			ppressed = true;
			oss << "Pen";
		}

		//oss << "point " << gPickedIndex;
		gMessage = oss.str();
	}

	// Uncomment these lines to see the picking shader in effect
	//glfwSwapBuffers(window);
	//continue; // skips the normal rendering
}

int initWindow(void)
{
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Naugle,Nicolas(1626-9133)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);

	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	return 0;
}

void initOpenGL(void)
{

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	gViewMatrix = glm::lookAt(glm::vec3(10.0, 10.0, 10.0f),	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		glm::vec3(0.0, 1.0, 0.0));	// up

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("Picking.vertexshader", "Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");
	
	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	LightID2 = glGetUniformLocation(programID, "LightPosition_worldspace2");

	createObjects();
}

void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {

	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);
	const size_t Normaloffset = sizeof(Vertices[0].Color) + RgbOffset;

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);	//
	glBindVertexArray(VertexArrayId[ObjectId]);		//

	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);


	// Create Buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset); 
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)Normaloffset);

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal

	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(
		2,                                // attribute
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);


	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
			);
	}
}

void cleanup(void)
{
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// ATTN: MODIFY AS APPROPRIATE
	/*if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_B:
			bpressed = true;
			cpressed = false;
		case GLFW_KEY_C:
			bpressed = false;
			cpressed = true;
		case GLFW_KEY_A:
			break;
		case GLFW_KEY_D:
			break;
		case GLFW_KEY_W:
			break;
		case GLFW_KEY_S:
			break;
		case GLFW_KEY_SPACE:
			break;
		default:
			break;
		}
	}*/
	if (key == GLFW_KEY_B) {
		bpressed = true;
		cpressed = false;
		tpressed = false;
		onepressed = false;
		twopressed = false;
		ppressed = false;
	}
	if (key == GLFW_KEY_C) {
		bpressed = false;
		cpressed = true;
		tpressed = false;
		onepressed = false;
		twopressed = false;
		ppressed = false;
	}
	if (key == GLFW_KEY_T) {
		bpressed = false;
		cpressed = false;
		tpressed = true;
		onepressed = false;
		twopressed = false;
		ppressed = false;
		float newColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		//VertexArrayId[3] = new Vertex 
		//for (int i = 0; i < sizeof(Top_Verts); i++) {
			//Top_Verts[i].SetColor(newColor);// .SetColor(newColor);
		//}
		
		
	}
	if (key == GLFW_KEY_1) {
		bpressed = false;
		cpressed = false;
		tpressed = false;
		onepressed = true;
		twopressed = false;
		ppressed = false;
	}
	if (key == GLFW_KEY_2) {
		bpressed = false;
		cpressed = false;
		tpressed = false;
		onepressed = false;
		twopressed = true;
		ppressed = false;
	}
	if (key == GLFW_KEY_P) {
		bpressed = false;
		cpressed = false;
		tpressed = false;
		onepressed = false;
		twopressed = false;
		ppressed = true;
	}


	//if c pressed and keys down
	if (cpressed && key == GLFW_KEY_LEFT) {
		move_camera(0, 1);
	}
	else if (cpressed && key == GLFW_KEY_RIGHT) {
		move_camera(0, -1);
	}
	else if (cpressed && key == GLFW_KEY_UP) {
		move_camera(1, 0);
	}
	else if (cpressed && key == GLFW_KEY_DOWN) {
		move_camera(-1, 0);
	}


	//if b pressed and keys down
	if (bpressed && key == GLFW_KEY_LEFT) {
		if (baseX > -5.0) {
			baseX = baseX - 0.2;
		}
	}
	else if (bpressed && key == GLFW_KEY_RIGHT) {
		if (baseX < 5.0) {
			baseX = baseX + 0.2;
		}
	}
	else if (bpressed && key == GLFW_KEY_UP) {
		if (baseZ > -5.0) {
			baseZ = baseZ - 0.2;
		}
	}
	else if (bpressed && key == GLFW_KEY_DOWN) {
		if (baseZ < 5.0) {
			baseZ = baseZ + 0.2;
		}
	}


	//if b pressed and keys down
	if (tpressed && key == GLFW_KEY_LEFT) {
		top_rotation = top_rotation - 0.1;
	}
	else if (tpressed && key == GLFW_KEY_RIGHT) {
		top_rotation = top_rotation + 0.1;
	}



	//if 1 pressed and keys down
	if (onepressed && key == GLFW_KEY_DOWN) {
		if (arm1_rotate > -2.0) {
			arm1_rotate = arm1_rotate - 0.1;
		}
	}
	else if (onepressed && key == GLFW_KEY_UP) {
		if (arm1_rotate < 2.0) {
			arm1_rotate = arm1_rotate + 0.1;
		}
	}


	//if 1 pressed and keys down
	if (twopressed && key == GLFW_KEY_DOWN) {
		if (joint_rotation > -1.0) {
			joint_rotation = joint_rotation - 0.1;
		}
	}
	else if (twopressed && key == GLFW_KEY_UP) {
		if (joint_rotation < 1.0) {
			joint_rotation = joint_rotation + 0.1;
		}
	}

	//if p pressed and keys down
	if (ppressed && key == GLFW_KEY_LEFT && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		|| (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
		pen_rotation = pen_rotation - 0.2;
	}
	else if (ppressed && key == GLFW_KEY_RIGHT && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		|| (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
		pen_rotation = pen_rotation + 0.2;
	}
	else if (ppressed && key == GLFW_KEY_LEFT) {
		arm2_rotation = arm2_rotation - 0.2;
	}
	else if (ppressed && key == GLFW_KEY_RIGHT) {
		arm2_rotation = arm2_rotation + 0.2;
	}
	else if (ppressed && key == GLFW_KEY_UP) {
		if (button_rotation > 0.5) {
			button_rotation = button_rotation - 0.2;
		}
	}
	else if (ppressed && key == GLFW_KEY_DOWN) {
		if (button_rotation < 2.75) {
			button_rotation = button_rotation + 0.2;
		}
	}
	

}


void move_camera(int x, int y) {
	
	thetax = thetax + (0.1f * x);
	thetay = thetay + (0.1f * y);
	
	mat4 rotationx = {
		{1.0, 0.0,         0.0,          0.0},
		{0.0, cos(thetax), -sin(thetax), 0.0},
		{0.0, sin(thetax), cos(thetax),  0.0},
		{0.0, 0.0,         0.0,          1.0}
	};

	mat4 rotationy = {
		{ cos(thetay),  0.0, sin(thetay),  0.0 },
		{ 0.0,          1.0, 0.0,          0.0 },
		{ -sin(thetay), 0.0, cos(thetay),  0.0 },
		{ 0.0,          0.0, 0.0,          1.0 }
	};

	mat4 rotationz = {
		{ cos(thetay), -sin(thetay), 0.0, 0.0},
		{ sin(thetay), cos(thetay), 0.0, 0.0},
		{0.0, 0.0, 1.0, 0.0},
		{0.0, 0.0, 0.0, 1.0}
	};

	vec4 ones = { 1.0, 1.0, 1.0, 1.0 };
	vec4 cameralocations = rotationy * rotationx * ones;
	vec3 inter = { cameralocations[0] * 10, cameralocations[1] * 10, cameralocations[2] * 10 };
	
	camerax = cameralocations[0] * 10;
	cameray = cameralocations[1] * 10;
	cameraz = cameralocations[2] * 10;

	gViewMatrix = glm::lookAt(inter,	// eye
		glm::vec3(0.0, 0.0, 0.0),	// look at this location
		glm::vec3(0.0, 1.0, 0.0));	// up

}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickObject();
	}
}


int main(void)
{
	// initialize window
	bpressed = false;
	cpressed = false;
	tpressed = false;
	onepressed = false;
	twopressed = false;
	ppressed = false;

	camerax = 10.0;
	cameray = 10.0;
	cameraz = 10.0;
	thetax = 1;
	thetay = 1;

	baseX = 0;
	baseZ = 0;




	top_rotation = 0.0;
	arm1_rotate = -pi/2;
	joint_rotation = pi/4;
	arm2_rotation = pi/2;
	button_rotation = pi/2;
	pen_rotation = 0.5;

	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// initialize OpenGL pipeline
	initOpenGL();

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		//// Measure speed
		//double currentTime = glfwGetTime();
		//nbFrames++;
		//if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1sec ago
		//	// printf and reset
		//	printf("%f ms/frame\n", 1000.0 / double(nbFrames));
		//	nbFrames = 0;
		//	lastTime += 1.0;
		//}
		
		if (animation){
			phi += 0.01;
			if (phi > 360)
				phi -= 360;
		}

		// DRAWING POINTS
		renderScene();


	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}