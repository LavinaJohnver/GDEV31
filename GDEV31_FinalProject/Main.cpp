#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>
#include <cfloat>
#include <functional>

/**
 * @brief Path to the input file
 * 
 * IMPORTANT: If you are using Mac, kindly change this to the full path of the test.txt file
 */
#define INPUT_FILE "test.txt"

/**
 * @brief 2D point representation
 */
struct Point {
    /** X-coordinate */
    float x;

    /** Y-coordinate */
    float y;
};

/**
 * @brief Represents a Voronoi cell for a given site
 */
struct Cell {
    /** Site position */
    Point site;

    /** Vertices forming the boundary of the cell */
    std::vector<Point> vertices;

    /** Placeholder edge color */
    float edgeColor[3] = {1.0f, 1.0f, 1.0f};
};

/**
 * @brief Site group used in test cases
 */
struct Sites {
    std::vector<Point> sitelist;
};

/**
 * @brief Constructs Voronoi cells for a given set of site points.
 * 
 * @param sites List of site points
 * @return std::vector<Cell> Generated Voronoi cells
 * 
 * 
 */
std::vector<Cell> VoronoiDiagram(std::vector<Point>& sites) {
	// Helper types and functions -------------------------------------------------
	struct Line {
		Point p; // point on the line (midpoint)
		Point v; // normal/direction vector (from site to other)
	};

	float (*dot)(const Point&, const Point&) = [](const Point& a, const Point& b) -> float {
		return a.x * b.x + a.y * b.y;
	};

	Point (*sub)(const Point&, const Point&) = [](const Point& a, const Point& b) -> Point {
		return Point{ a.x - b.x, a.y - b.y };
	};

	Point (*add)(const Point&, const Point&) = [](const Point& a, const Point& b) -> Point {
		return Point{ a.x + b.x, a.y + b.y };
	};

	Point (*mul)(const Point&, float) = [](const Point& a, float s) -> Point {
		return Point{ a.x * s, a.y * s };
	};

	std::function<bool(const Point&, const Point&, const Line&, float&)> IntersectSegmentWithLine = [&](const Point& a, const Point& b, const Line& L, float& outT) -> bool {
		// Solve for t in a + t*(b-a) such that ( (a + t*(b-a) - L.p) dot L.v ) = 0
		Point ab = sub(b, a);
		float denom = dot(ab, L.v);
		if (fabsf(denom) < 1e-9f) return false; // parallel
		outT = dot(sub(L.p, a), L.v) / denom;
		return (outT >= -1e-6f && outT <= 1.0f + 1e-6f);
	};

	// Sutherland-Hodgman polygon clipping against half-plane defined by 'clipLine'.
	std::function<std::vector<Point>(const std::vector<Point>&, const Line&, const Point&)> ClipPolygonByLine = [&](const std::vector<Point>& polygon, const Line& clipLine, const Point& /*insideSite*/) -> std::vector<Point> {
		std::vector<Point> out;
		if (polygon.empty()) return out;

		std::function<bool(const Point&)> isInside = [&](const Point& q) -> bool {
			// inside if (q - clipLine.p) dot clipLine.v <= 0 => q is on the side of the site
			return dot(sub(q, clipLine.p), clipLine.v) <= 1e-6f;
		};

		Point A = polygon.back();
		bool Ainside = isInside(A);
		for (size_t i = 0; i < polygon.size(); ++i) {
			Point B = polygon[i];
			bool Binside = isInside(B);

			if (Ainside && Binside) {
				out.push_back(B);
			}
			else if (Ainside && !Binside) {
				float t;
				if (IntersectSegmentWithLine(A, B, clipLine, t)) {
					out.push_back( add(A, mul(sub(B, A), t)) );
				}
			}
			else if (!Ainside && Binside) {
				float t;
				if (IntersectSegmentWithLine(A, B, clipLine, t)) {
					out.push_back( add(A, mul(sub(B, A), t)) );
				}
				out.push_back(B);
			}
			A = B; Ainside = Binside;
		}

		return out;
	};

	std::function<std::vector<Point>(const Point&, const std::vector<Point>&, float, float, float, float)> ComputeCellVertices = [&](const Point& site, const std::vector<Point>& allSites,
							   float minX, float maxX, float minY, float maxY) -> std::vector<Point> {
		std::vector<Point> cell;
		// rectangle CCW
		cell.push_back({ minX, minY });
		cell.push_back({ maxX, minY });
		cell.push_back({ maxX, maxY });
		cell.push_back({ minX, maxY });

		for (const Point& other : allSites) {
			if (fabsf(other.x - site.x) < 1e-6f && fabsf(other.y - site.y) < 1e-6f) continue;

			Point m = { (site.x + other.x) * 0.5f, (site.y + other.y) * 0.5f };
			Point v = sub(other, site); // normal toward the other site

			Line bis; bis.p = m; bis.v = v;

			cell = ClipPolygonByLine(cell, bis, site);
			if (cell.empty()) break;
		}

		return cell;
	};

	std::vector<Cell> voronoiCells;
	int WINDOW_WIDTH = 1280;
	int WINDOW_HEIGHT = 720;
	float minX = 0.0f, minY = 0.0f, maxX = WINDOW_WIDTH * 1.0f, maxY = WINDOW_HEIGHT * 1.0f;
	for (size_t i = 0; i < sites.size(); ++i) {
		Cell cell;
		cell.site = sites[i];
		cell.vertices = ComputeCellVertices(sites[i], sites, minX, maxX, minY, maxY);
		cell.edgeColor[0] = 0.9f; cell.edgeColor[1] = 0.9f; cell.edgeColor[2] = 0.9f;
		voronoiCells.push_back(cell);
	}

	return voronoiCells;
}


// ----------------------------------------------------------------------------------
// RENDER CODE. If you need to edit any of the code here, feel free to do so.
// ----------------------------------------------------------------------------------

#include <cstddef>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <stdexcept>


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
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
struct AppData {
	std::vector<Sites> testCases;
	int currentTestCase;
	GLuint vbo;
	size_t numVertices;
	Point player;
	int playerCellIndex;
};
struct Vertex {
	GLfloat x, y, z;
	GLfloat r, g, b;
};
struct Matrix4x4 {
	GLfloat values[16];
};

void RefreshScene(AppData* data);

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

	//EDIT THIS CODE IF YOU WANT EXTRA KEYBOARD INPUTS

	AppData* appData = reinterpret_cast<AppData*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) {
			appData->currentTestCase = ((appData->currentTestCase - 1) + appData->testCases.size()) % appData->testCases.size();
			RefreshScene(appData);
		}
		else if (key == GLFW_KEY_RIGHT) {
			appData->currentTestCase = (appData->currentTestCase + 1) % appData->testCases.size();
			RefreshScene(appData);
		}
	}
	
	// Handle WASD movement (continuous while held)
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		float moveSpeed = 5.0f;
		bool moved = false;
		
		if (key == GLFW_KEY_W || key == GLFW_KEY_UP) {
			appData->player.y += moveSpeed;
			moved = true;
		}
		if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN) {
			appData->player.y -= moveSpeed;
			moved = true;
		}
		if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) {
			appData->player.x -= moveSpeed;
			moved = true;
		}
		if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) {
			appData->player.x += moveSpeed;
			moved = true;
		}
		
		// Clamp player to window bounds
		if (appData->player.x < 0) appData->player.x = 0;
		if (appData->player.x > WINDOW_WIDTH) appData->player.x = WINDOW_WIDTH;
		if (appData->player.y < 0) appData->player.y = 0;
		if (appData->player.y > WINDOW_HEIGHT) appData->player.y = WINDOW_HEIGHT;
		
		if (moved) {
			RefreshScene(appData);
		}
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

// The RefreshScene() function draws the sites and cells of the Voronoi, so if you want to
// make edits to the render, edit that function.

void RefreshScene(AppData* data) {

	//EDIT THIS CODE FOR ANY NEW RENDER

	std::vector<Vertex> vertices;
	// Draw sites as a circle
	Sites& sites = data->testCases[data->currentTestCase];
	
	// Get the data of the voronoi cells
	std::vector<Cell> voronoiCells = VoronoiDiagram(data->testCases[data->currentTestCase].sitelist);
	
	// Helper to check if point is inside polygon
	std::function<bool(const Point&, const std::vector<Point>&)> pointInPolygon = [](const Point& p, const std::vector<Point>& poly) -> bool {
		if (poly.size() < 3) return false;
		int crossings = 0;
		for (size_t i = 0; i < poly.size(); ++i) {
			const Point& v0 = poly[i];
			const Point& v1 = poly[(i + 1) % poly.size()];
			if (((v0.y <= p.y && p.y < v1.y) || (v1.y <= p.y && p.y < v0.y)) &&
				(p.x < (v1.x - v0.x) * (p.y - v0.y) / (v1.y - v0.y) + v0.x)) {
				crossings++;
			}
		}
		return (crossings % 2) == 1;
	};
	
	// Find which cell contains the player
	data->playerCellIndex = -1;
	for (size_t i = 0; i < voronoiCells.size(); ++i) {
		if (pointInPolygon(data->player, voronoiCells[i].vertices)) {
			data->playerCellIndex = (int)i;
			break;
		}
	}
	
	// Draw Voronoi cells
	for (size_t cellIdx = 0; cellIdx < voronoiCells.size(); ++cellIdx) {
		Cell& cell = voronoiCells[cellIdx];
		bool isPlayerCell = ((int)cellIdx == data->playerCellIndex);
		
		// Draw filled polygon if player is in this cell
		if (isPlayerCell && cell.vertices.size() >= 3) {
			for (size_t i = 1; i + 1 < cell.vertices.size(); ++i) {
				Vertex v0 = { cell.vertices[0].x, cell.vertices[0].y, -0.5f, 0.2f, 0.4f, 0.8f };
				Vertex v1 = { cell.vertices[i].x, cell.vertices[i].y, -0.5f, 0.2f, 0.4f, 0.8f };
				Vertex v2 = { cell.vertices[i + 1].x, cell.vertices[i + 1].y, -0.5f, 0.2f, 0.4f, 0.8f };
				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(v2);
			}
		}
		
		// Draw cell edges
		float r = cell.edgeColor[0];
		float g = cell.edgeColor[1];
		float b = cell.edgeColor[2];
		
		// Highlighted edges for player cell
		if (isPlayerCell) {
			r = 0.3f; g = 0.6f; b = 1.0f;
		}
		
		for (size_t i = 0; i < cell.vertices.size(); ++i) {
			Point p0 = cell.vertices[i];
			Point p1 = cell.vertices[(i + 1) % cell.vertices.size()];
			AppendLineVertices(p0, p1, 2.0f, vertices, 0.0f, r, g, b);
		}
	}
	
	// Draw sites as white circles
	for (size_t i = 0; i < sites.sitelist.size(); ++i) {
		AppendCircleVertices(sites.sitelist[i], 5.0f, vertices, 0.2f, 1.0f, 1.0f, 1.0f);
	}
	
	// Draw player as a red circle
	AppendCircleVertices(data->player, 7.0f, vertices, 0.3f, 1.0f, 0.2f, 0.2f);

	glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	data->numVertices = vertices.size();
}

int main(int argc, char* argv[]) {
	srand(time(nullptr));

	AppData appData = {};
	appData.player = { WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f };
	appData.playerCellIndex = -1;
	std::ifstream file(INPUT_FILE);
	if (file.fail())
	{
		std::cout << "Failed to read file test.txt" << std::endl;
		return 1;
	}

	size_t numTestCases;
	file >> numTestCases;
	for (size_t testCaseNumber = 0; testCaseNumber < numTestCases; ++testCaseNumber) {
		Sites sites;
		
		size_t numPoints;
		file >> numPoints;

		std::vector<Point> points;
		for (size_t j = 0; j < numPoints; ++j) {
			Point point;
			file >> point.x >> point.y;
			points.push_back(point);
		}
		sites.sitelist = points;

		appData.testCases.push_back(sites);
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

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "EspirituLaviñaVillanueva - Voronoi Diagram", nullptr, nullptr);
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

	RefreshScene(&appData);

	Matrix4x4 projMatrix = CreateOrtho(0.0f, WINDOW_WIDTH * 1.0f, 0.0f, WINDOW_HEIGHT * 1.0f, -100.0f, 100.0f);
	float prevTime = glfwGetTime(), speed = 200.0f;
	while (!glfwWindowShouldClose(window)) {
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - prevTime;
		prevTime = currentTime;

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
