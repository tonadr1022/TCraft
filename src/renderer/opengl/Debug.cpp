#include "Debug.hpp"

#include "pch.hpp"

// adapted from https://learnopengl.com/In-Practice/Debugging
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                [[maybe_unused]] GLsizei length, const GLchar* message,
                                [[maybe_unused]] const void* userParam) {
  // ignore insignificant error/warning codes
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

  std::stringstream err_stream;
  err_stream << "OpenGL Debug message (" << id << "): " << message << '\n';

  switch (source) {
    case GL_DEBUG_SOURCE_API:
      err_stream << "Source: API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      err_stream << "Source: Window Manager";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      err_stream << "Source: Shader Compiler";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      err_stream << "Source: Third Party";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      err_stream << "Source: Application";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      err_stream << "Source: Other";
      break;
    default:
      break;
  }

  err_stream << '\n';

  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      err_stream << "Type: Error";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      err_stream << "Type: Deprecated Behaviour";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      err_stream << "Type: Undefined Behaviour";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      err_stream << "Type: Portability";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      err_stream << "Type: Performance";
      break;
    case GL_DEBUG_TYPE_MARKER:
      err_stream << "Type: Marker";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
      err_stream << "Type: Push Group";
      break;
    case GL_DEBUG_TYPE_POP_GROUP:
      err_stream << "Type: Pop Group";
      break;
    case GL_DEBUG_TYPE_OTHER:
      err_stream << "Type: Other";
      break;
    default:
      break;
  }

  err_stream << '\n';

  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      err_stream << "Severity: high";
      spdlog::critical("{}", err_stream.str());
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      err_stream << "Severity: medium";
      spdlog::error("{}", err_stream.str());
      break;
    case GL_DEBUG_SEVERITY_LOW:
      err_stream << "Severity: low";
      spdlog::warn("{}", err_stream.str());
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      err_stream << "Severity: notification";
      spdlog::debug("{}", err_stream.str());
      break;
    default:
      break;
  }
}
