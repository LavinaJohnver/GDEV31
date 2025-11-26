#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
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

struct Line {
	float a, b, c; // ax + by + c = 0
};

/**
 * @brief Represents a Voronoi cell for a given site
 */
struct Cell {
    /** Site position */
    Point site;

    /** Vertices forming the boundary of the cell */
    std::vector<Point> vertices;

	/** Edge color */
	float edgeColor[3] = {1.0f, 1.0f, 1.0f};

	/** Fill color */
	float fillColor[3] = {0.2f, 0.2f, 0.2f};
};

/**
 * @brief Site group used in test cases
 */
struct Sites {
    std::vector<Point> sitelist;
};


// Get the perpendicular line bisecting s1 and s2.
Line GetPerpBisector(const Point& s1, const Point& s2) {
	// Midpoint of two sites
	Point mid {
		(s1.x + s2.x) * 0.5f,
		(s1.y + s2.y) * 0.5f
	};

	// Direction Vector
	float dx = s2.x - s1.x;
	float dy = s2.y - s1.y;

	// Perpendicular Vector to DirectionVector
	float px = -dy; // a
	float py = dx; // b

	// Get c by manipulating the line equation ax + by + c = 0
	// c = -(ax + by)
	float c = -(px*mid.x + py*mid.y);

	return {px, py, c};
}


// Checks if two lines are intersecting.
// If true, out will have the point of intersection.
bool IsLineIntersecting(const Line& l1, const Line& l2, Point& out) {
	// Use cross product to check if the lines are parallel.
	// Lines are parallel if cross product is 0.
	// If parallel, return false.
	float cross = l1.a * l2.b - l2.a * l1.b;
    if (fabs(cross) == 0)
        return false;

	// If not parallel, set out to be the intersection.
    out.x = (l1.b * l2.c - l2.b * l1.c) / cross;
    out.y = (l1.a * l2.c - l2.a * l1.c) / cross;
    return true;
}


// Get the distance between two points.
float PointsDistance(const Point& p1, const Point& p2) {
	float dx = p2.x - p1.x;
	float dy = p2.y - p1.y;
	return sqrt(dx*dx + dy*dy);
}

// Takes a point and two site and checks if 
// the point is closer to the first site. 
bool IsCloserToSite(const Point& p, const Point& s1, const Point& s2) {
	float d1 = PointsDistance(p, s1);
	float d2 = PointsDistance(p, s2);
	if (d1 < d2) return true; 
	else return false;
}

// Takes a center point and a list of points.
// Arranges them in the list to go counterclockwise
// around the center. This ensures creating a 
// cohesive polygon shape.
std::vector<Point> SortVertices(const Point& center, std::vector<Point> verts) {
	int n = verts.size();
	for (int i = 0; i < n; i++) {
		// Assume i is the smallest angle.
		int minIndex = i;
		float angleMin = atan2(verts[i].y - center.y, verts[i].x - center.x); // Get the angle.

		for (int j = i+1; j < n; j++) {
			float newAngle = atan2(verts[j].y - center.y, verts[j].x - center.x); // Get the angle.
			// If the new angle is smaller, replace as the smallest angle.
			if (newAngle < angleMin) {
				minIndex = j;
				angleMin = newAngle;
			}
		}
		// Put the smallest angle at index i.
		if (minIndex != i) {
			std::swap(verts[i], verts[minIndex]);
		}
	}
    return verts;
}


// Takes 2 points and checks if their line is within the bounds
// of the window (minX, maxX, minY, maxY). If it is within the
// window, clip the line so it fits inside.
bool ClipLine(
	const Point& p1, const Point& p2, // input points for the line
	float minX, float maxX, float minY, float maxY, // bounds of the window
	Point& clipped1, Point& clipped2 // output points for the line
    ) 
{
	// Direction Vector
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

	// Entry and Exit Points
	// Represents % of the line that 
	// should be in the window.
    float tEnter = 0.0f;
    float tExit = 1.0f;

	// Direction of the line relative to the boundaries
	// of the window.
    float p[] = {-dx, dx, -dy, dy}; // left, right, bottom, top

	// Distance from p1 to the boundary.
    float q[] = {p1.x - minX, maxX - p1.x, p1.y - minY, maxY - p1.y};

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0) { // line is parallel to the boundary
            if (q[i] < 0) { // line is outside the boundary
				return false;
			}
        } else {
            float t = q[i] / p[i]; // % along the line segment
            if (p[i] < 0) {
				tEnter = std::max(tEnter, t); 
			}
            else {
				tExit = std::min(tExit, t);
			}
        }
    }

	// If line is not in the window, return false.
    if (tEnter > tExit) return false;

	// Otherwise, this is your new line segment that fits within the window.
    clipped1.x = p1.x + (tEnter * dx);
    clipped1.y = p1.y + (tEnter * dy);
    clipped2.x = p1.x + (tExit * dx);
    clipped2.y = p1.y + (tExit * dy);
    return true;
}

// HSV to RGB conversion helper function
void HSVtoRGB(float h, float s, float v, float* rgb) {
    float c = v * s;
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h >= 0 && h < 60) {
        r = c; g = x; b = 0;
    } else if (h >= 60 && h < 120) {
        r = x; g = c; b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0; g = c; b = x;
    } else if (h >= 180 && h < 240) {
        r = 0; g = x; b = c;
    } else if (h >= 240 && h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    rgb[0] = r + m;
    rgb[1] = g + m;
    rgb[2] = b + m;
}

// Generate unique color for each Voronoi region
void GenerateUniqueColor(int index, int total, float* color) {
    // Calculate hue based on index to spread colors evenly across spectrum
    float hue = fmod(index * 360.0f / total, 360.0f);
    
    // Use fixed saturation and value for vibrant, distinct colors
    float saturation = 0.7f;
    float value = 0.9f;
    
    // Convert HSV to RGB
    HSVtoRGB(hue, saturation, value, color);
}


/**
 * @brief Constructs Voronoi cells for a given set of site points.
 * 
 * @param sites List of site points
 * @return std::vector<Cell> Generated Voronoi cells
 */
std::vector<Cell> VoronoiDiagram(std::vector<Point>& sites) {
	int WINDOW_WIDTH = 1280;
	int WINDOW_HEIGHT = 720;
	float minX = 0.0f, maxX = WINDOW_WIDTH * 1.0f;
	float minY = 0.0f, maxY = WINDOW_HEIGHT * 1.0f;
	
	std::vector<Cell> voronoiCells;
	
	// Lambda helper functions for vector operations
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
	
	// Helper struct for half-plane clipping
	struct HalfPlane {
		Point p;  // point on the line (midpoint)
		Point n;  // normal vector (pointing toward the site we're keeping)
	};
	
	// Intersect a line segment with a half-plane boundary
	std::function<bool(const Point&, const Point&, const HalfPlane&, float&)> intersectSegment = [&](const Point& a, const Point& b, const HalfPlane& hp, float& t) -> bool {
		Point ab = sub(b, a);
		float denom = dot(ab, hp.n);
		if (fabsf(denom) < 1e-9f) return false;
		t = dot(sub(hp.p, a), hp.n) / denom;
		return (t >= -1e-6f && t <= 1.0f + 1e-6f);
	};
	
	// Sutherland-Hodgman clipping against half-plane
	std::function<std::vector<Point>(const std::vector<Point>&, const HalfPlane&)> clipPolygon = [&](const std::vector<Point>& polygon, const HalfPlane& hp) -> std::vector<Point> {
		std::vector<Point> output;
		if (polygon.empty()) return output;
		
		std::function<bool(const Point&)> isInside = [&](const Point& q) -> bool {
			return dot(sub(q, hp.p), hp.n) <= 1e-6f;
		};
		
		Point prev = polygon.back();
		bool prevInside = isInside(prev);
		
		for (const Point& curr : polygon) {
			bool currInside = isInside(curr);
			
			if (prevInside && currInside) {
				output.push_back(curr);
			}
			else if (prevInside && !currInside) {
				float t;
				if (intersectSegment(prev, curr, hp, t)) {
					output.push_back(add(prev, mul(sub(curr, prev), t)));
				}
			}
			else if (!prevInside && currInside) {
				float t;
				if (intersectSegment(prev, curr, hp, t)) {
					output.push_back(add(prev, mul(sub(curr, prev), t)));
				}
				output.push_back(curr);
			}
			
			prev = curr;
			prevInside = currInside;
		}
		
		return output;
	};
	
	// Compute cell for each site
	for (size_t i = 0; i < sites.size(); ++i) {
		Cell cell;
		cell.site = sites[i];
		
		// Start with window rectangle (counter-clockwise)
		std::vector<Point> polygon;
		polygon.push_back({minX, minY});
		polygon.push_back({maxX, minY});
		polygon.push_back({maxX, maxY});
		polygon.push_back({minX, maxY});
		
		// Clip against each other site's perpendicular bisector
		for (size_t j = 0; j < sites.size(); ++j) {
			if (i == j) continue;
			
			// Midpoint and normal (pointing from j toward i)
			Point mid = { (sites[i].x + sites[j].x) * 0.5f, (sites[i].y + sites[j].y) * 0.5f };
			Point normal = sub(sites[j], sites[i]);  // Points toward j, so we keep the i side
			
			HalfPlane hp;
			hp.p = mid;
			hp.n = normal;
			
			polygon = clipPolygon(polygon, hp);
			if (polygon.empty()) break;
		}
		
		cell.vertices = polygon;
		
		// Generate unique color for this cell (fill), set edges RED by default
		GenerateUniqueColor(i, sites.size(), cell.fillColor);
		cell.edgeColor[0] = 1.0f; cell.edgeColor[1] = 0.0f; cell.edgeColor[2] = 0.0f;
		
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
		
		if (key == GLFW_KEY_W ) {
			appData->player.y += moveSpeed;
			moved = true;
		}
		if (key == GLFW_KEY_S ) {
			appData->player.y -= moveSpeed;
			moved = true;
		}
		if (key == GLFW_KEY_A ) {
			appData->player.x -= moveSpeed;
			moved = true;
		}
		if (key == GLFW_KEY_D ) {
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

	// Build a fresh vertex list each refresh
	std::vector<Vertex> vertices;

	// Current test case sites
	Sites& sites = data->testCases[data->currentTestCase];

	// Generate Voronoi cells for current test case
	std::vector<Cell> voronoiCells = VoronoiDiagram(sites.sitelist);

	// Point-in-polygon (ray casting) to locate the player's cell
	std::function<bool(const Point&, const std::vector<Point>&)> pointInPolygon = [](const Point& p, const std::vector<Point>& poly) -> bool {
		if (poly.empty()) return false;
		int crossings = 0;
		size_t n = poly.size();
		for (size_t i = 0; i < n; ++i) {
			const Point& a = poly[i];
			const Point& b = poly[(i + 1) % n];
			bool condY = ((a.y > p.y) != (b.y > p.y));
			if (condY) {
				float t = (p.y - a.y) / (b.y - a.y);
				float xIntersect = a.x + t * (b.x - a.x);
				if (xIntersect > p.x) crossings++;
			}
		}
		return (crossings % 2) == 1;
	};

	// Determine which cell the player is in
	data->playerCellIndex = -1;
	for (size_t i = 0; i < voronoiCells.size(); ++i) {
		if (pointInPolygon(data->player, voronoiCells[i].vertices)) {
			data->playerCellIndex = static_cast<int>(i);
			break;
		}
	}

	// Draw each Voronoi cell: filled with unique color, edges white; highlight player cell with yellow edges
	const float EDGE_WIDTH = 2.0f;
	for (size_t i = 0; i < voronoiCells.size(); ++i) {
		Cell& cell = voronoiCells[i];
		const std::vector<Point>& poly = cell.vertices;
		if (poly.size() < 3) continue; // need at least a triangle to fill

		// Compute centroid for fan triangulation
		Point centroid{0.0f, 0.0f};
		for (const Point& v : poly) { centroid.x += v.x; centroid.y += v.y; }
		centroid.x /= poly.size();
		centroid.y /= poly.size();

		// Fill fan (brighten fill if player is inside this cell)
		float fr = cell.fillColor[0];
		float fg = cell.fillColor[1];
		float fb = cell.fillColor[2];
		if (static_cast<int>(i) == data->playerCellIndex) {
			float t = 0.4f; // blend toward white for emphasis
			fr = fr * (1.0f - t) + 1.0f * t;
			fg = fg * (1.0f - t) + 1.0f * t;
			fb = fb * (1.0f - t) + 1.0f * t;
		}
		for (size_t k = 0; k < poly.size(); ++k) {
			const Point& a = poly[k];
			const Point& b = poly[(k + 1) % poly.size()];
			vertices.push_back({ centroid.x, centroid.y, -0.5f, fr, fg, fb });
			vertices.push_back({ a.x, a.y, -0.5f, fr, fg, fb });
			vertices.push_back({ b.x, b.y, -0.5f, fr, fg, fb });
		}

		// Edge color (yellow if player is inside this cell)
		float er = cell.edgeColor[0];
		float eg = cell.edgeColor[1];
		float eb = cell.edgeColor[2];
		if (static_cast<int>(i) == data->playerCellIndex) { er = 1.0f; eg = 0.0f; eb = 0.0f; }
		for (size_t k = 0; k < poly.size(); ++k) {
			const Point& a = poly[k];
			const Point& b = poly[(k + 1) % poly.size()];
			AppendLineVertices(a, b, EDGE_WIDTH, vertices, 0.0f, er, eg, eb);
		}
	}

	// Draw sites (small circles) on top of edges
	for (size_t i = 0; i < sites.sitelist.size(); ++i) {
		float r = 1.0f, g = 1.0f, b = 1.0f;
		if (static_cast<int>(i) == data->playerCellIndex) { r = 1.0f; g = 1.0f; b = 0.0f; } // highlight site of current cell
		AppendCircleVertices(sites.sitelist[i], 4.5f, vertices, 0.25f, r, g, b);
	}

	// Draw player (larger circle) - red
	AppendCircleVertices(data->player, 6.0f, vertices, 0.5f, 1.0f, 0.0f, 0.0f);

	// Upload to GPU
	glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
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
