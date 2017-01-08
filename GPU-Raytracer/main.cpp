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
#include "Utility\colour.hpp"
#include "Utility\gl.hpp"
#include "Utility\quaternion.hpp"

/** If you change these values, change it in the shaders as well. **/
const static int MAX_LIGHTS = 5;
const static int MAX_SPHERES = 5;
const static int MAX_PLANES = 5;

struct Light
{
	Point3 pos;
	Colour colour;
};

struct Sphere
{
	Point3 center;
	Vector3 velocity;

	int radius = 0;
};

struct Plane
{
	Vector3 normal;
	float d;
};

struct RaytracerShader : public Shader
{
	Uniform light_uniform = -1;
	Uniform light_count_uniform = -1;

	Uniform sphere_uniform = -1;
	Uniform sphere_count_uniform = -1;

	Uniform plane_uniform = -1;
	Uniform plane_count_uniform = -1;

	Uniform origin_uniform = -1;
	Uniform camera_matrix_uniform = -1;
};

struct State
{
	int window = 0;

	RaytracerShader shader;

	Light lights[MAX_LIGHTS];
	int active_lights = 0;

	Sphere spheres[MAX_SPHERES];
	int active_spheres = 0;

	Plane planes[MAX_PLANES];
	int active_planes = 0;

	Quaternion rotation;
};

State state;

void update() {
	for (int i = 0; i < state.active_spheres; ++i) {
		Sphere &sphere = state.spheres[i];
		sphere.center += sphere.velocity;
		if (sphere.center.x - sphere.radius < -200) {
			sphere.center.x = -200 + sphere.radius;
			sphere.velocity.x *= -0.99;
		} else if (sphere.center.x + sphere.radius > 200) {
			sphere.center.x = 200 - sphere.radius;
			sphere.velocity.x *= -0.99;
		}
		if (sphere.center.y - sphere.radius < -200) {
			sphere.center.y = -200 + sphere.radius;
			sphere.velocity.y *= -0.99;
		} else if (sphere.center.y + sphere.radius > 200) {
			sphere.center.y = 200 - sphere.radius;
			sphere.velocity.y *= -0.99;
		}
		if (sphere.center.z - sphere.radius < -200) {
			sphere.center.z = -200 + sphere.radius;
			sphere.velocity.z *= -0.99;
		} else if (sphere.center.z + sphere.radius > 200) {
			sphere.center.z = 200 - sphere.radius;
			sphere.velocity.z *= -0.99;
		}
	}
	//state.rotation *= Quaternion(0.0, 0.003, 0.0, 1.0);
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(state.shader.program);

	// Serialize the lights.
	GLfloat *light_data = new GLfloat[state.active_lights * 4];
	for (int i = 0; i < state.active_lights; ++i) {
		const Light &light = state.lights[i];
		int index = i * 4;
		light_data[index + 0] = light.pos.x;
		light_data[index + 1] = light.pos.y;
		light_data[index + 2] = light.pos.z;
		light_data[index + 3] = 0;
	}

	glUniform4fv(state.shader.light_uniform, state.active_spheres, light_data);
	glUniform1i(state.shader.light_count_uniform, state.active_lights);

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

	glUniform4fv(state.shader.sphere_uniform, state.active_spheres, sphere_data);
	glUniform1i(state.shader.sphere_count_uniform, state.active_spheres);

	// Serialize the planes.
	GLfloat *plane_data = new GLfloat[state.active_planes * 4];
	for (int i = 0; i < state.active_planes; ++i) {
		const Plane &plane = state.planes[i];
		int index = i * 4;
		plane_data[index + 0] = plane.normal.x;
		plane_data[index + 1] = plane.normal.y;
		plane_data[index + 2] = plane.normal.z;
		plane_data[index + 3] = plane.d;
	}

	glUniform4fv(state.shader.plane_uniform, state.active_planes, plane_data);
	glUniform1i(state.shader.plane_count_uniform, state.active_planes);

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
	state.shader.light_uniform = glGetUniform(state.shader, "lights");
	state.shader.light_count_uniform = glGetUniform(state.shader, "light_count");
	state.shader.sphere_uniform = glGetUniform(state.shader, "spheres");
	state.shader.sphere_count_uniform = glGetUniform(state.shader, "sphere_count");
	state.shader.plane_uniform = glGetUniform(state.shader, "planes");
	state.shader.plane_count_uniform = glGetUniform(state.shader, "plane_count");
	state.shader.origin_uniform = glGetUniform(state.shader, "origin");
	state.shader.camera_matrix_uniform = glGetUniform(state.shader, "camera_matrix");
}

void cleanup() {
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	state.lights[0].pos = Point3(0, 0, 0);
	state.lights[1].pos = Point3(0, 0, -600);
	state.active_lights = 2;

	for (int i = 0; i < MAX_SPHERES; ++i) {
		state.spheres[i].center.x = Randf(-200, 200);
		state.spheres[i].center.y = Randf(-200, 200);
		state.spheres[i].center.z = Randf(-200, 200);
		state.spheres[i].velocity.x = Randf(-3, 3);
		state.spheres[i].velocity.y = Randf(-3, 3);
		state.spheres[i].velocity.z = Randf(-3, 3);
		state.spheres[i].radius = Randf(25, 50);
	}
	state.active_spheres = MAX_SPHERES;

	state.planes[0].normal = Vector3(-1, 0, 0);
	state.planes[0].d = 200;
	state.planes[1].normal = Vector3(1, 0, 0);
	state.planes[1].d = 200;
	state.planes[2].normal = Vector3(0, 0, -1);
	state.planes[2].d = 200;
	state.planes[3].normal = Vector3(0, -1, 0);
	state.planes[3].d = 200;
	state.planes[4].normal = Vector3(0, 1, 0);
	state.planes[4].d = 200;
	state.active_planes = 5;

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