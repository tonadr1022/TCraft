#include "EventDispatcher.hpp"

void EventDispatcher::AddListener(const EventHandler& handler) { listeners_.emplace_back(handler); }

bool EventDispatcher::Dispatch(const SDL_Event& event) {
  // return std::any_of(listeners_.begin(), listeners_.end(),
  //                    [&](EventHandler& listener) { return listener(event); });

  for (const auto& listener : listeners_) {
    if (listener(event)) return true;
  }
  return false;
}
