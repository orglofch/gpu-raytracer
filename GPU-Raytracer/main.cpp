/*
* Copyright (c) 2016 Owen Glofcheski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
*    1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
*
*    2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
*    3. This notice may not be removed or altered from any source
*    distribution.
*/

#include <ctime>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <vector>

#include "Utility\algebra.hpp"
#include "Utility\gl.hpp"
#include "Utility\quaternion.hpp"

/** If you change this value, change it in the shaders as well. **/
const static int MAX_SPHERES = 200;

struct Sphere
{
	Point3 center;
	int radius = 0;
};

struct RaytracerShader : public Shader
{
	Uniform sphere_uniform = -1;
	Uniform sphere_count_uniform = -1;

	Uniform origin_uniform = -1;
	Uniform camera_matrix_uniform = -1;
};

struct State
{
	int window = 0;

	RaytracerShader shader;

	Sphere spheres[MAX_SPHERES];
	int active_spheres = 0;

	Quaternion rotation;
};

State state;

void update() {
	state.rotation *= Quaternion(0.0, 0.003, 0.0, 1.0);
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	// Serialize the spheres.
	GLfloat *sphere_data = new GLfloat[state.active_spheres * 4];
	for (int i = 0; i < state.active_spheres; ++i) {
		const Sphere &sphere = state.spheres[i];
		int index = i * 4;
		sphere_data[index + 0] = sphere.center.x;
		sphere_data[index + 1] = sphere.center.y;
		sphere_data[index + 2] = sphere.center.z;
		sphere_data[index + 3] = sphere.radius;
	}

	glUseProgram(state.shader.program);
	glUniform4fv(state.shader.sphere_uniform, state.active_spheres, sphere_data);
	glUniform1i(state.shader.sphere_count_uniform, state.active_spheres);

	Point3 eye = state.rotation.matrix() * Point3(0, 0, -600);

	GLfloat origin[3];
	for (int i = 0; i < 3; ++i) {
		origin[i] = eye.d[i];
	}
	glUniform3fv(state.shader.origin_uniform, 1, origin);

	Vector3 view = Point3(0, 0, 0) - eye;
	view.normalize();
	Vector3 up(0, 1, 0);

	double d = view.length();
	double h = 2.0 * d * tan(toRad(60 /* fov */) / 2.0);
	Matrix4x4 t1 = Matrix4x4::translation(-540, -360, d);
	Matrix4x4 s2 = Matrix4x4::scaling(-h / 720, -h / 720, 1.0);
	Matrix4x4 r3 = Matrix4x4::rotation(eye, view, up);
	Matrix4x4 t4 = Matrix4x4::translation(eye - Point3(0, 0, 0));
	Matrix4x4 camera_matrix = t4 * r3 * s2 * t1;
	GLfloat camera_data[16];
	for (int i = 0; i < 16; ++i) {
		camera_data[i] = camera_matrix.d[i];
	}
	glUniformMatrix4fv(state.shader.camera_matrix_uniform, 1, false, camera_data);

	delete[] sphere_data;
	glDrawRect(-1, 1, -1, 1, 0);

	glutSwapBuffers();
	glutPostRedisplay();
}

void tick() {
	update();
	render();
}

void handleMouseButton(int button, int button_state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			break;
		case GLUT_RIGHT_BUTTON:
			break;
	}
}

void handlePressNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'q':
		case 'Q':
		case 27:
			exit(EXIT_SUCCESS);
	}
}

void handleReleaseNormalKeys(unsigned char key, int x, int y) {
}

void handlePressSpecialKey(int key, int x, int y) {
}

void handleReleaseSpecialKey(int key, int x, int y) {
}


void init() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glutDisplayFunc(tick);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(handleMouseButton);
	glutKeyboardFunc(handlePressNormalKeys);
	glutKeyboardUpFunc(handleReleaseNormalKeys);
	glutSpecialFunc(handlePressSpecialKey);
	glutSpecialUpFunc(handleReleaseSpecialKey);

	state.shader.program = glLoadShader("raytracer.vert", "raytracer.frag");
	state.shader.sphere_uniform = glGetUniform(state.shader, "spheres");
	state.shader.sphere_count_uniform = glGetUniform(state.shader, "sphere_count");
	state.shader.origin_uniform = glGetUniform(state.shader, "origin");
	state.shader.camera_matrix_uniform = glGetUniform(state.shader, "camera_matrix");
}

void cleanup() {
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	for (int i = 0; i < MAX_SPHERES; ++i) {
		state.spheres[i].center.x = Randf(-200, 200);
		state.spheres[i].center.y = Randf(-200, 200);
		state.spheres[i].center.z = Randf(-200, 200);
		state.spheres[i].radius = Randf(5, 50);
	}
	state.active_spheres = MAX_SPHERES;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1080, 720);
	state.window = glutCreateWindow("GPU Raytracer");
	glewInit();

	init();

	glutMainLoop();

	cleanup();
}