#include "pho_thread.h"
#include <algorithm>

namespace pho {

// EventQueue implementation

void EventQueue::push(InputEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(event));
}

bool EventQueue::pop(InputEvent& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    out = std::move(queue_.front());
    queue_.pop();
    return true;
}

bool EventQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t EventQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void EventQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) queue_.pop();
}

// EngineRunLoop implementation

EngineRunLoop::EngineRunLoop() = default;

void EngineRunLoop::post_event(InputEvent event) {
    events_.push(std::move(event));
}

void EngineRunLoop::set_event_handler(EventHandler handler) {
    event_handler_ = std::move(handler);
}

uint32_t EngineRunLoop::tick(double current_time) {
    current_time_ = current_time;
    uint32_t count = 0;

    // Process all pending events
    InputEvent event;
    while (events_.pop(event)) {
        if (event_handler_) {
            event_handler_(event);
        }
        count++;
    }

    // Fire ready timers
    std::vector<std::function<void()>> to_fire;
    {
        std::lock_guard<std::mutex> lock(timer_mutex_);
        for (auto it = timers_.begin(); it != timers_.end(); ) {
            if (current_time >= it->fire_time) {
                to_fire.push_back(it->callback);
                if (it->repeating && it->interval > 0) {
                    it->fire_time = current_time + it->interval;
                    ++it;
                } else {
                    it = timers_.erase(it);
                }
            } else {
                ++it;
            }
        }
    }
    for (auto& fn : to_fire) {
        fn();
        count++;
    }

    return count;
}

uint32_t EngineRunLoop::schedule_timer(double delay, double interval, bool repeating,
                                        std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    uint32_t id = next_timer_id_++;
    timers_.push_back({id, current_time_ + delay, interval, repeating, std::move(callback)});
    return id;
}

void EngineRunLoop::cancel_timer(uint32_t timer_id) {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
                        [timer_id](const TimerEntry& t) { return t.id == timer_id; }),
        timers_.end());
}

} // namespace pho
