#include "EventDispatcher.hpp"

#include <algorithm>

void EventDispatcher::AddListener(const EventHandler& handler) { listeners_.emplace_back(handler); }

bool EventDispatcher::Dispatch(const SDL_Event& event) {
  return std::ranges::any_of(listeners_,
                             [&event](EventHandler& listener) { return listener(event); });
}
