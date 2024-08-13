#include "application/Window.hpp"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_video.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <glm/vec2.hpp>

#include "pch.hpp"
#include "spdlog/spdlog.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

namespace {

SDL_GLContext gl_context;

void SetImGuiStyle() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
  // colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  // colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  // colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
  colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
  colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
  // colors[ImGuiCol_MenuBarBg]              = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.2, 0.2, 0.2, 1.000f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
  colors[ImGuiCol_Separator] = ImVec4(0.4f, 0.4f, 0.4f, 0.137f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

  style.PopupRounding = 1;

  style.WindowPadding = ImVec2(4, 4);
  style.FramePadding = ImVec2(6, 4);
  style.ItemSpacing = ImVec2(6, 4);

  style.ScrollbarSize = 8;

  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 0;

  style.WindowRounding = 1;
  style.ChildRounding = 1;
  style.FrameRounding = 1;
  style.ScrollbarRounding = 1;
  style.GrabRounding = 1;
}
}  // namespace

Window* Window::instance_ = nullptr;
Window::Window() { instance_ = this; }
Window& Window::Get() { return *instance_; }

// https://github.com/ocornut/imgui/blob/bb224c8aa1de1992c6ea3483df56fb04d6d1b5b6/examples/example_sdl2_opengl3/main.cpp
void Window::Init(int width, int height, const char* title, const EventCallback& event_callback) {
  event_callback_ = event_callback;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    spdlog::critical("Error: {}\n", SDL_GetError());
  }
  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char* glsl_version = "#version 100";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
  // GL 3.2 Core + GLSL 150
  const char* glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  const char* glsl_version = "#version 430";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
#endif

  // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  auto window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                                   SDL_WINDOW_ALLOW_HIGHDPI);
  window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                             window_flags);
  gl_context = SDL_GL_CreateContext(window_);
  SDL_GL_MakeCurrent(window_, gl_context);
  SetVsync(true);

  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  SetImGuiStyle();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window_, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  GLenum err = glewInit();
  if (err != GLEW_OK) {
    /* Problem: glewInit failed, something is seriously wrong. */
    const char* err_string = reinterpret_cast<const char*>(glewGetErrorString(err));
    spdlog::critical("GLEW initialization failed: {}", err_string);
  }

  // const char* version = reinterpret_cast<const char*>(glewGetString(GLEW_VERSION));
  // spdlog::info("Using GLEW version: {}", version);
}
void Window::PollEvents() {
  ZoneScoped;
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants
  // to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application,
  // or clear/overwrite your copy of the mouse data.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
  // application, or clear/overwrite your copy of the keyboard data. Generally you may always pass
  // all inputs to dear imgui, and hide them from your application based on those two flags.
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type) {
      case SDL_QUIT:
        should_close_ = true;
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window_)) {
          should_close_ = true;
        }
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if (ImGui::GetIO().WantCaptureKeyboard) break;
        event_callback_(event);
        break;
      case SDL_MOUSEMOTION:
        if (ImGui::GetIO().WantCaptureMouse) break;
        event_callback_(event);
        break;
      case SDL_MOUSEWHEEL:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        event_callback_(event);
      default:
        break;
    }
  }
}

void Window::StartRenderFrame(bool imgui_enabled) {
  ZoneScoped;
  if (imgui_enabled) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
  }
}

void Window::EndRenderFrame(bool imgui_enabled) const {
  {
    ZoneScopedN("ImGui render");
    if (imgui_enabled) {
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
  }
  {
    ZoneScopedN("Swap buffers");
    SDL_GL_SwapWindow(window_);
  }
}

void Window::Shutdown() {
  ZoneScoped;
  // Cleanup
  {
    ZoneScopedN("imgui destroy");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  }
  {
    ZoneScopedN("sdl gl");
    SDL_GL_DeleteContext(gl_context);
  }
  {
    ZoneScopedN("sdl window");
    SDL_DestroyWindow(window_);
    SDL_Quit();
  }
}
void Window::CenterCursor() {
  auto dims = GetWindowSize();
  SDL_WarpMouseInWindow(window_, static_cast<float>(dims.x) / 2, static_cast<float>(dims.y) / 2);
}

void Window::SetMouseGrab(bool state) {
  SDL_SetWindowMouseGrab(window_, static_cast<SDL_bool>(state));
}

bool Window::ShouldClose() const { return should_close_; }

void Window::SetVsync(bool vsync) {
  vsync_on_ = vsync;
  SDL_GL_SetSwapInterval(vsync);
}

bool Window::GetVsync() const { return vsync_on_; }

std::string Window::GetEventTypeString(Uint32 eventType) {
  static const std::unordered_map<Uint32, std::string> EventTypeMap = {
      {SDL_QUIT, "SDL_QUIT"},
      {SDL_APP_TERMINATING, "SDL_APP_TERMINATING"},
      {SDL_APP_LOWMEMORY, "SDL_APP_LOWMEMORY"},
      {SDL_APP_WILLENTERBACKGROUND, "SDL_APP_WILLENTERBACKGROUND"},
      {SDL_APP_DIDENTERBACKGROUND, "SDL_APP_DIDENTERBACKGROUND"},
      {SDL_APP_WILLENTERFOREGROUND, "SDL_APP_WILLENTERFOREGROUND"},
      {SDL_APP_DIDENTERFOREGROUND, "SDL_APP_DIDENTERFOREGROUND"},
      {SDL_WINDOWEVENT, "SDL_WINDOWEVENT"},
      {SDL_SYSWMEVENT, "SDL_SYSWMEVENT"},
      {SDL_KEYDOWN, "SDL_KEYDOWN"},
      {SDL_KEYUP, "SDL_KEYUP"},
      {SDL_TEXTEDITING, "SDL_TEXTEDITING"},
      {SDL_TEXTINPUT, "SDL_TEXTINPUT"},
      {SDL_KEYMAPCHANGED, "SDL_KEYMAPCHANGED"},
      {SDL_MOUSEMOTION, "SDL_MOUSEMOTION"},
      {SDL_MOUSEBUTTONDOWN, "SDL_MOUSEBUTTONDOWN"},
      {SDL_MOUSEBUTTONUP, "SDL_MOUSEBUTTONUP"},
      {SDL_MOUSEWHEEL, "SDL_MOUSEWHEEL"},
      {SDL_JOYAXISMOTION, "SDL_JOYAXISMOTION"},
      {SDL_JOYBALLMOTION, "SDL_JOYBALLMOTION"},
      {SDL_JOYHATMOTION, "SDL_JOYHATMOTION"},
      {SDL_JOYBUTTONDOWN, "SDL_JOYBUTTONDOWN"},
      {SDL_JOYBUTTONUP, "SDL_JOYBUTTONUP"},
      {SDL_JOYDEVICEADDED, "SDL_JOYDEVICEADDED"},
      {SDL_JOYDEVICEREMOVED, "SDL_JOYDEVICEREMOVED"},
      {SDL_CONTROLLERAXISMOTION, "SDL_CONTROLLERAXISMOTION"},
      {SDL_CONTROLLERBUTTONDOWN, "SDL_CONTROLLERBUTTONDOWN"},
      {SDL_CONTROLLERBUTTONUP, "SDL_CONTROLLERBUTTONUP"},
      {SDL_CONTROLLERDEVICEADDED, "SDL_CONTROLLERDEVICEADDED"},
      {SDL_CONTROLLERDEVICEREMOVED, "SDL_CONTROLLERDEVICEREMOVED"},
      {SDL_CONTROLLERDEVICEREMAPPED, "SDL_CONTROLLERDEVICEREMAPPED"},
      {SDL_FINGERDOWN, "SDL_FINGERDOWN"},
      {SDL_FINGERUP, "SDL_FINGERUP"},
      {SDL_FINGERMOTION, "SDL_FINGERMOTION"},
      {SDL_DOLLARGESTURE, "SDL_DOLLARGESTURE"},
      {SDL_DOLLARRECORD, "SDL_DOLLARRECORD"},
      {SDL_MULTIGESTURE, "SDL_MULTIGESTURE"},
      {SDL_CLIPBOARDUPDATE, "SDL_CLIPBOARDUPDATE"},
      {SDL_DROPFILE, "SDL_DROPFILE"},
      {SDL_DROPTEXT, "SDL_DROPTEXT"},
      {SDL_DROPBEGIN, "SDL_DROPBEGIN"},
      {SDL_DROPCOMPLETE, "SDL_DROPCOMPLETE"},
      {SDL_AUDIODEVICEADDED, "SDL_AUDIODEVICEADDED"},
      {SDL_AUDIODEVICEREMOVED, "SDL_AUDIODEVICEREMOVED"},
      {SDL_RENDER_TARGETS_RESET, "SDL_RENDER_TARGETS_RESET"},
      {SDL_RENDER_DEVICE_RESET, "SDL_RENDER_DEVICE_RESET"}};

  auto it = EventTypeMap.find(eventType);
  if (it != EventTypeMap.end()) {
    return it->second;
  }
  return "Unknown event";
}

glm::ivec2 Window::GetWindowSize() const {
  int x;
  int y;
  SDL_GetWindowSize(window_, &x, &y);
  return {x, y};
}

glm::ivec2 Window::GetMousePosition() const {
  int x;
  int y;
  SDL_GetMouseState(&x, &y);
  return {x, y};
}

glm::ivec2 Window::GetWindowCenter() const { return GetWindowSize() / 2; }

void Window::DisableImGuiInputs() {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
  io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

// Function to enable ImGui input handling
void Window::EnableImGuiInputs() {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

float Window::GetAspectRatio() const {
  auto dims = GetWindowSize();
  return static_cast<float>(dims.x) / static_cast<float>(dims.y);
}

void Window::SetTitle(std::string_view title) { SDL_SetWindowTitle(window_, title.data()); }
