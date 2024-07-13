#pragma once

#include <SDL_events.h>

#include <functional>

class EventDispatcher {
 public:
  using EventHandler = std::function<bool(const SDL_Event&)>;

  void AddListener(const EventHandler& handler);
  bool Dispatch(const SDL_Event& event);

 private:
  std::vector<EventHandler> listeners_;
};
