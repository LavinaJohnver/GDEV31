#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>

/**
 * Point class
 */
struct Point {
	/**
	 * X-coordinate of the point
	 */
	float x;

	/**
	 * Y-coordinate of the point
	 */
	float y;
};

/**
 * @brief Calculates the perpendicular distance from a point to a line segment
 * @param point The point to measure distance from
 * @param lineStart Start point of the line segment
 * @param lineEnd End point of the line segment
 * @return The perpendicular distance
 */
float perpendicularDistance(const Point& point, const Point& lineStart, const Point& lineEnd) {
    float dx = lineEnd.x - lineStart.x;
    float dy = lineEnd.y - lineStart.y;
    
    // If the line segment is actually a point
    float lineLengthSquared = dx * dx + dy * dy;
    if (lineLengthSquared == 0) {
        float px = point.x - lineStart.x;
        float py = point.y - lineStart.y;
        return std::sqrt(px * px + py * py);
    }
    
    // Calculate perpendicular distance using the cross product formula
    // Distance = |cross product| / |line length|
    float numerator = std::abs(dy * point.x - dx * point.y + lineEnd.x * lineStart.y - lineEnd.y * lineStart.x);
    float denominator = std::sqrt(lineLengthSquared);
    
    return numerator / denominator;
}

/**
 * @brief Helper function for the Ramer-Douglas-Peucker algorithm
 * @param points Vector containing the points of the polyline
 * @param startIdx Start index in the points vector
 * @param endIdx End index in the points vector
 * @param epsilon Maximum distance between the two polylines
 * @param keep Vector to mark which points to keep
 */
void RDPHelper(const std::vector<Point>& points, int startIdx, int endIdx, float epsilon, std::vector<bool>& keep) {
    if (endIdx <= startIdx + 1) {
        return;
    }
    
    // Find the point with maximum distance from the line segment
    float maxDistance = 0.0f;
    int maxIndex = startIdx;
    
    for (int i = startIdx + 1; i < endIdx; i++) {
        float distance = perpendicularDistance(points[i], points[startIdx], points[endIdx]);
        if (distance > maxDistance) {
            maxDistance = distance;
            maxIndex = i;
        }
    }
    
    // If max distance is greater than epsilon, recursively simplify
    if (maxDistance > epsilon) {
        keep[maxIndex] = true;
        RDPHelper(points, startIdx, maxIndex, epsilon, keep);
        RDPHelper(points, maxIndex, endIdx, epsilon, keep);
    }
}

/**
 * @brief Simplifies the provided polyline using the Ramer-Douglas-Peucker algorithm.
 * @param polylinePoints Vector containing the points of the polyline
 * @param epsilon Maximum distance between the two polylines
 * @return Returns a vector containing the points of the simplified polyline
 */
std::vector<Point> RamerDouglasPeucker(const std::vector<Point>& polylinePoints, float epsilon) {
    if (polylinePoints.size() <= 2) {
        return polylinePoints;
    }
    
    // Mark which points to keep
    std::vector<bool> keep(polylinePoints.size(), false);
    
    // Always keep first and last points
    keep[0] = true;
    keep[polylinePoints.size() - 1] = true;
    
    // Run the recursive algorithm
    RDPHelper(polylinePoints, 0, polylinePoints.size() - 1, epsilon, keep);
    
    // Build the result vector with only the kept points
    std::vector<Point> result;
    for (size_t i = 0; i < polylinePoints.size(); i++) {
        if (keep[i]) {
            result.push_back(polylinePoints[i]);
        }
    }
    
    return result;
}
// ----------------------------------------------------------------------------------
// DO NOT TOUCH ANY CODE BEYOND THIS POINT
// ----------------------------------------------------------------------------------
#include <cstddef>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

const char* vertShaderSource = R"(#version 330
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
out vec3 outVertexColor;
uniform mat4 mvpMatrix;
void main() {
    gl_Position = mvpMatrix * vec4(vertexPosition, 1.0);
	outVertexColor = vertexColor;
})";
const char* fragShaderSource = R"(#version 330
in vec3 outVertexColor;
out vec4 fragColor;
void main() {
	fragColor = vec4(outVertexColor, 1.0);
})";
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct AppData {
	std::vector<Point> polyline;
	GLuint vbo;
	float epsilon;
	size_t numVertices;
	bool bShowOriginalPolyline;
};
struct Vertex {
	GLfloat x, y, z;
	GLfloat r, g, b;
};
struct Matrix4x4 {
	GLfloat values[16];
};

void RefreshScene(AppData* data);
void GenerateRandomPolyline(std::vector<Point>& points);

Matrix4x4 operator*(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 ret;
	ret.values[0] = a.values[0] * b.values[0] + a.values[4] * b.values[1] + a.values[8] * b.values[2] + a.values[12] * b.values[3];
	ret.values[4] = a.values[0] * b.values[4] + a.values[4] * b.values[5] + a.values[8] * b.values[6] + a.values[12] * b.values[7];
	ret.values[8] = a.values[0] * b.values[8] + a.values[4] * b.values[9] + a.values[8] * b.values[10] + a.values[12] * b.values[11];
	ret.values[12] = a.values[0] * b.values[12] + a.values[4] * b.values[13] + a.values[8] * b.values[14] + a.values[12] * b.values[15];
	ret.values[1] = a.values[1] * b.values[0] + a.values[5] * b.values[1] + a.values[9] * b.values[2] + a.values[13] * b.values[3];
	ret.values[5] = a.values[1] * b.values[4] + a.values[5] * b.values[5] + a.values[9] * b.values[6] + a.values[13] * b.values[7];
	ret.values[9] = a.values[1] * b.values[8] + a.values[5] * b.values[9] + a.values[9] * b.values[10] + a.values[13] * b.values[11];
	ret.values[13] = a.values[1] * b.values[12] + a.values[5] * b.values[13] + a.values[9] * b.values[14] + a.values[13] * b.values[15];
	ret.values[2] = a.values[2] * b.values[0] + a.values[6] * b.values[1] + a.values[10] * b.values[2] + a.values[14] * b.values[3];
	ret.values[6] = a.values[2] * b.values[4] + a.values[6] * b.values[5] + a.values[10] * b.values[6] + a.values[14] * b.values[7];
	ret.values[10] = a.values[2] * b.values[8] + a.values[6] * b.values[9] + a.values[10] * b.values[10] + a.values[14] * b.values[11];
	ret.values[14] = a.values[2] * b.values[12] + a.values[6] * b.values[13] + a.values[10] * b.values[14] + a.values[14] * b.values[15];
	ret.values[3] = a.values[3] * b.values[0] + a.values[7] * b.values[1] + a.values[11] * b.values[2] + a.values[15] * b.values[3];
	ret.values[7] = a.values[3] * b.values[4] + a.values[7] * b.values[5] + a.values[11] * b.values[6] + a.values[15] * b.values[7];
	ret.values[11] = a.values[3] * b.values[8] + a.values[7] * b.values[9] + a.values[11] * b.values[10] + a.values[15] * b.values[11];
	ret.values[15] = a.values[3] * b.values[12] + a.values[7] * b.values[13] + a.values[11] * b.values[14] + a.values[15] * b.values[15];

	return ret;
}

GLuint CreateShader(const GLuint& type, const std::string& source) {
	GLuint shader = glCreateShader(type);

	const char* sourceCStr = source.c_str();
	GLint sourceLen = source.size();
	glShaderSource(shader, 1, &sourceCStr, &sourceLen);
	glCompileShader(shader);

	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		char infoLog[512];
		GLsizei infoLogLen = sizeof(infoLog);
		glGetShaderInfoLog(shader, infoLogLen, &infoLogLen, infoLog);

		std::string errorMsg;
		if (type == GL_VERTEX_SHADER) {
			errorMsg += std::string("Failed to compile vertex shader!\n");
		}
		else if (type == GL_FRAGMENT_SHADER) {
			errorMsg += std::string("Failed to compile fragment shader!\n");
		}
		else {
			errorMsg += std::string("Failed to compile shader!\n");
		}
		errorMsg += std::string(infoLog);

		std::cout << errorMsg << std::endl;
	}

	return shader;
}

GLuint CreateShaderProgramFromSource(const std::string& vertexShaderSource, const std::string& fragmentShaderSource) {
	GLuint vsh = CreateShader(GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fsh = CreateShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	GLuint program = glCreateProgram();
	glAttachShader(program, vsh);
	glAttachShader(program, fsh);
	glLinkProgram(program);

	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE) {
		char infoLog[512];
		GLsizei infoLogLen = sizeof(infoLog);
		glGetProgramInfoLog(program, infoLogLen, &infoLogLen, infoLog);
		throw std::runtime_error(std::string("program link error: ") + infoLog);
		return 0;
	}

	glDetachShader(program, vsh);
	glDetachShader(program, fsh);
	glDeleteShader(vsh);
	glDeleteShader(fsh);

	return program;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	AppData* appData = reinterpret_cast<AppData*>(glfwGetWindowUserPointer(window));
	if ((action == GLFW_PRESS) && (key == GLFW_KEY_SPACE)) {
		appData->polyline.clear();
		GenerateRandomPolyline(appData->polyline);
		RefreshScene(appData);
	}
	else if ((action == GLFW_PRESS) && (key == GLFW_KEY_LEFT)) {
		appData->epsilon -= 1.0f;
		appData->epsilon = (appData->epsilon < 0.0f) ? 0.0f : appData->epsilon;
		RefreshScene(appData);
		std::cout << "Epsilon: " << appData->epsilon << std::endl;
	}
	else if ((action == GLFW_PRESS) && (key == GLFW_KEY_RIGHT)) {
		appData->epsilon += 1.0f;
		appData->epsilon = (appData->epsilon > 50.0f) ? 50.0f : appData->epsilon;
		RefreshScene(appData);
		std::cout << "Epsilon: " << appData->epsilon << std::endl;
	}
	else if ((action == GLFW_PRESS) && (key == GLFW_KEY_P)) {
		appData->bShowOriginalPolyline = !appData->bShowOriginalPolyline;
		RefreshScene(appData);
	}
}

Matrix4x4 CreateIdentity() {
	Matrix4x4 ret = {};
	ret.values[0] = 1.0f; ret.values[5] = 1.0f; ret.values[10] = 1.0f; ret.values[15] = 1.0f;
	return ret;
}

Matrix4x4 CreateOrtho(float left, float right, float bottom, float top, float near, float far) {
	Matrix4x4 ret =
	{
		2.0f / (right - left), 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
		0.0f, 0.0f, -2.0f / (far - near), 0.0f,
		-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f
	};

	return ret;
}

void AppendLineVertices(const Point& p0, const Point& p1, float width, std::vector<Vertex>& vertices, float zOrder = 0.0f, float r = 1.0f, float g = 1.0f, float b = 1.0f) {
	float vx = p1.x - p0.x, vy = p1.y - p0.y;
	float vm = vx * vx + vy * vy;
	if (fabsf(vm) > 1e-9f) {
		vm = sqrtf(vm);
		vx = vx / vm; vy = vy / vm;
	}
	float nx = -vy, ny = vx;

	Vertex v0 = { p0.x + nx * width / 2.0f, p0.y + ny * width / 2.0f, zOrder, r, g, b };
	Vertex v1 = { p0.x - nx * width / 2.0f, p0.y - ny * width / 2.0f, zOrder, r, g, b };
	Vertex v2 = { p1.x - nx * width / 2.0f, p1.y - ny * width / 2.0f, zOrder, r, g, b };
	Vertex v3 = { p1.x + nx * width / 2.0f, p1.y + ny * width / 2.0f, zOrder, r, g, b };

	vertices.push_back( v0 ); vertices.push_back( v1 ); vertices.push_back( v2 );
	vertices.push_back( v2 ); vertices.push_back( v3 ); vertices.push_back( v0 );
}

void AppendCircleVertices(const Point& center, float radius, std::vector<Vertex>& vertices, float zOrder = 0.0f, float r = 1.0f, float g = 1.0f, float b = 1.0f) {
	int numSections = 180;
	float anglePerSection = 360.0f / numSections * M_PI / 180.0f;
	for (int i = 1; i <= numSections; ++i) {
		float x0 = center.x + radius * cosf(anglePerSection * (i - 1));
		float y0 = center.y + radius * sinf(anglePerSection * (i - 1));
		float x1 = center.x + radius * cosf(anglePerSection * i);
		float y1 = center.y + radius * sinf(anglePerSection * i);
		vertices.push_back( { center.x, center.y, zOrder, r, g, b } );
		vertices.push_back( { x0, y0, zOrder, r, g, b } );
		vertices.push_back( { x1, y1, zOrder, r, g, b } );
	}
}

void GeneratePolyline(const Point& start, const Point& end, int numSteps, float maxEpsilon, std::vector<Point>& points) {
	points.push_back(start);

	float vx = end.x - start.x;
	float vy = end.y - start.y;
	float nx = -vy, ny = vx;
	float vm = vx * vx + vy * vy;
	if (fabsf(vm) > 1e-9f) {
		vm = sqrtf(vm);
		nx = nx / vm; ny = ny / vm;
	}

	float t = 0.0f;
	for (int i = 1; i <= numSteps; ++i) {
		float r = (rand() % 5 + 1) / 100.0f;
		t += r;
		if (t >= 1.0f) {
			break;
		}

		Point p = { start.x + vx * t, start.y + vy * t };
		
		r = rand() % 101 - 50;
		float epsilon = maxEpsilon * (r / 50.0f);
		p.x += nx * epsilon;
		p.y += ny * epsilon;

		points.push_back(p);
	}
	points.push_back(end);
}

void GenerateRandomPolyline(std::vector<Point>& points) {
	const int NUM_POINTS = 2;
	static Point startPoints[NUM_POINTS] = { { 20.0f, 150.0f }, { 20.0f, 400.0f } };
	static Point endPoints[NUM_POINTS] = { { 750.0f, 300.0f }, { 750.0f, 100.0f } };
	int r = rand() % NUM_POINTS;
	GeneratePolyline( startPoints[r], endPoints[r], 30, 70.0f, points );
}

void RefreshScene(AppData* data) {
	std::vector<Point> simplifiedPolyline = RamerDouglasPeucker(data->polyline, data->epsilon);

	std::vector<Vertex> vertices;
	for (int i = 0; i < data->polyline.size(); ++i) {
		AppendCircleVertices( data->polyline[i], 4.0f, vertices );
		if (data->bShowOriginalPolyline) {
			if (i > 0) {
				AppendLineVertices(data->polyline[i - 1], data->polyline[i], 2.0f, vertices);
			}
		}
	}
	for (int i = 1; i < simplifiedPolyline.size(); ++i) {
		AppendLineVertices(simplifiedPolyline[i - 1], simplifiedPolyline[i], 2.0f, vertices, 1.0f, 1.0f, 0.0f, 0.0f);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	data->numVertices = vertices.size();
}

int main(int argc, char* argv[]) {
	srand(time(nullptr));

	AppData appData = {};
	appData.bShowOriginalPolyline = true;
	GenerateRandomPolyline(appData.polyline);
	appData.epsilon = 20.0f;
	bool bUI = true, bPrintPolyline = false, bPrintOutput = false;
	if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-i") == 0) {
				std::ifstream file(argv[++i]);
				if (!file.fail()) {
					appData.polyline.clear();
					int numPoints; file >> numPoints;
					for (int j = 0; j < numPoints; ++j) {
						Point p; file >> p.x >> p.y;
						appData.polyline.push_back(p);
					}
				}
			}
			else if (strcmp(argv[i], "-p") == 0) {
				bPrintPolyline = true;
			}
			else if (strcmp(argv[i], "-o") == 0) {
				bPrintOutput = true;
			}
			else if (strcmp(argv[i], "-e") == 0) {
				appData.epsilon = atof(argv[++i]);
			}
			else if (strcmp(argv[i], "-s") == 0) {
				bUI = false;
			}
		}

		if (bPrintPolyline) {
			std::cout << appData.polyline.size() << std::endl;
			for (size_t i = 0; i < appData.polyline.size(); ++i) {
				std::cout << appData.polyline[i].x << " " << appData.polyline[i].y << std::endl;
			}
		}
		if (bPrintOutput) {
			std::vector<Point> simplifiedPolyline = RamerDouglasPeucker(appData.polyline, appData.epsilon);
			std::cout << simplifiedPolyline.size() << " " << appData.epsilon << std::endl;
			for (size_t i = 0; i < simplifiedPolyline.size(); ++i) {
				std::cout << simplifiedPolyline[i].x << " " << simplifiedPolyline[i].y << std::endl;
			}
		}

		if (!bUI) {
			return 0;
		}
	}

	if (glfwInit() == GLFW_FALSE) {
		std::cerr << "Cannot initialize GLFW!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Polyline Simplification", nullptr, nullptr);
	if (!window) {
		std::cerr << "Cannot create window.";
		return -1;
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glfwSetKeyCallback(window, KeyCallback);

	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));
	glBindVertexArray(0);

	GLuint program = CreateShaderProgramFromSource(vertShaderSource, fragShaderSource);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	appData.vbo = vbo;
	glfwSetWindowUserPointer(window, &appData);

	std::cout << "Epsilon: " << appData.epsilon << std::endl;
	RefreshScene(&appData);

	Matrix4x4 projMatrix = CreateOrtho(0.0f, WINDOW_WIDTH * 1.0f, 0.0f, WINDOW_HEIGHT * 1.0f, -100.0f, 100.0f);
	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(program);

		glBindVertexArray(vao);
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMatrix"), 1, GL_FALSE, projMatrix.values);
		glDrawArrays(GL_TRIANGLES, 0, appData.numVertices);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteProgram(program); program = 0;
	glDeleteVertexArrays(1, &vao); vao = 0;
	glDeleteBuffers(1, &vbo); vbo = 0;
	glfwDestroyWindow(window);window = nullptr;
	glfwTerminate();

	return 0;
}
