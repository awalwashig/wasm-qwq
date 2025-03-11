#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <cmath>
#include <cstring>  // 用于 strcmp
#include <spine/spine.h>

#include <iostream>


class {
public:
	bool up = 0;
	bool down = 0;
	bool left = 0;
	bool right = 0;
}key;

// 全局变量：着色器程序以及三角形的平移偏移量
GLuint program;
float offsetX = 0.0f;
float offsetY = 0.0f;
//

const char* vertex_shader =
"#version 300 es\n"
"in vec4 position;\n"
"uniform mat4 transformMatrix;\n"  // 修改名称为 transformMatrix，包含旋转和平移
"void main() { gl_Position = transformMatrix * position; }";

const char* fragment_shader =
"#version 300 es\n"
"precision highp float;\n"
"out vec4 fragColor;\n"
"void main() { fragColor = vec4(1.0, 0.5, 0.2, 1.0); }";

// 编译着色器
GLuint compile_shader(GLenum type, const char* source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	return shader;
}

void init_gl() {
	// 创建并链接着色器程序
	GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
	GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glUseProgram(program);
}

// 键盘回调函数，用于更新三角形的平移偏移量
EM_BOOL key_callback_down(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
	if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
		if (strcmp(e->key, "ArrowUp") == 0) {
			key.up = true;
		}
		else if (strcmp(e->key, "ArrowDown") == 0) {
			key.down = true;
		}
		else if (strcmp(e->key, "ArrowLeft") == 0) {
			key.left = true;
		}
		else if (strcmp(e->key, "ArrowRight") == 0) {
			key.right = true;
		}
	}
	return EM_TRUE;
}

EM_BOOL key_callback_up(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
	if (eventType == EMSCRIPTEN_EVENT_KEYUP) {
		if (strcmp(e->key, "ArrowUp") == 0) {
			key.up = false;
		}
		else if (strcmp(e->key, "ArrowDown") == 0) {
			key.down = false;
		}
		else if (strcmp(e->key, "ArrowLeft") == 0) {
			key.left = false;
		}
		else if (strcmp(e->key, "ArrowRight") == 0) {
			key.right = false;
		}
	}
	return EM_TRUE;
}

void move() {
	if (key.up) {
		offsetY += 0.04f;
	}
	if (key.down) {
		offsetY -= 0.04f;
	}
	if (key.left) {
		offsetX -= 0.04f;
	}
	if (key.right) {
		offsetX += 0.04f;
	}
}

void draw_frame() {
	// 更新旋转角度
	static GLfloat rotation = 0.0f;
	rotation += 0.01f;
	float c = cos(rotation), s = sin(rotation);

	// 构造包含旋转与平移的变换矩阵（列主序，传入时使用 GL_FALSE，不做转置）
	// 原矩阵为：
	// |  c   -s   0   0 |
	// |  s    c   0   0 |
	// |  0    0   1   0 |
	// | offsetX offsetY 0 1 |
	float transformMatrix[16] = {
		 c, -s, 0, 0,
		 s,  c, 0, 0,
		 0,  0, 1, 0,
		 offsetX, offsetY, 0, 1
	};

	// 三角形顶点数据
	float vertices[] = {
		 0.0f,  0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f
	};

	// 清除屏幕
	glClear(GL_COLOR_BUFFER_BIT);

	// 将变换矩阵传入顶点着色器
	glUniformMatrix4fv(glGetUniformLocation(program, "transformMatrix"), 1, GL_FALSE, transformMatrix);

	// 创建并绑定 VBO
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// 获取属性位置、启用并设置顶点属性指针
	GLint posAttrib = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// 绘制三角形
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// 删除 VBO，避免内存泄漏（在简单示例中可省略或使用全局 VBO）
	glDeleteBuffers(1, &vbo);
}

int main() {
	EmscriptenWebGLContextAttributes attrs;
	emscripten_webgl_init_context_attributes(&attrs);
	attrs.majorVersion = 2;  // 对应 WebGL 2.0
	

	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#canvas", &attrs);
	emscripten_webgl_make_context_current(context);

	init_gl();

	// 注册键盘按下回调，监听全局窗口的键盘事件
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, key_callback_up);
	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, key_callback_down);
	//emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, key_callback);

	emscripten_set_main_loop([]() {
		draw_frame();
		emscripten_webgl_commit_frame();
		move();
		}, 0, 1);

	return 0;
}
