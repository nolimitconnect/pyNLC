//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

#include "VxTimedCallback.h"
#include <iostream>

//============================================================================
VxTimedCallback::VxTimedCallback()
    : stop_requested(false)
    , is_running(false)
    , callback_func(nullptr)
{}

//============================================================================
VxTimedCallback::~VxTimedCallback() {
    stop();
}

//============================================================================
void VxTimedCallback::setCallback(std::function<void()> func) {
    std::unique_lock<std::mutex> lock(mutex);
    callback_func = func;
}
//============================================================================
void VxTimedCallback::start(int interval_ms) {
    stop(); // Always stop an old timer thread first

    stop_requested = false;
    is_running = true; // Timer is now active
    
    active_thread = std::thread([this, interval_ms]() {
        std::unique_lock<std::mutex> lock(mutex);
        
        while (!stop_requested) {
            auto timeout = std::chrono::milliseconds(interval_ms);
            if (cv.wait_for(lock, timeout, [this] { return stop_requested.load(); })) {
                break; 
            }
            
            // Check if a valid callback was actually set
            if (callback_func) {
                lock.unlock();
                callback_func(); // Run the stored function
                lock.lock();
            }
        }
    });
}

//============================================================================
// Shortcut to set the callback and start all at once
void VxTimedCallback::start(int interval_ms, std::function<void()> func) {
    setCallback(func);
    start(interval_ms);
}

//============================================================================
void VxTimedCallback::stop() {
    stop_requested = true;
    is_running = false; // Immediately mark as inactive
    cv.notify_all(); 
    
    if (active_thread.joinable()) {
        if (active_thread.get_id() == std::this_thread::get_id()) {
            // stop() can be invoked from inside the timer callback itself.
            // Joining the current thread throws resource_deadlock_would_occur.
            active_thread.detach();
        } else {
            active_thread.join();
        }
    }
}

//============================================================================
bool VxTimedCallback::isActive() const {
    return is_running.load();
}