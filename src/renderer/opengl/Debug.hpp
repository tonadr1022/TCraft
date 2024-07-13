#pragma once

// adapted from https://learnopengl.com/In-Practice/Debugging
extern void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                            const GLchar* message, const void* userParam);
