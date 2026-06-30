#pragma once
//============================================================================
// Copyright (C) 2026 Brett R. Jones
//
// Code copyrighted by Brett R. Jones is under dual license similar to Ruby's license
// See file COPYING and LEGAL in root of the No Limit Connect project
//
// bjones.engineer@gmail.com
// https://nolimitconnect.com
//============================================================================

// How to Use in a AudioMgr Class
// void AudioMgr::init() {
//     myTimer.setCallback([this]() { 
//         this->callbackAudioTestTimer(); 
//     });
// }

// // Somewhere else in your code when you want to run it:
// void AudioMgr::startMyTimer() {
//     myTimer.start(500); // Only needs 1 parameter now!
// }

// OR just start the timer and set the callback at the same time:
// 'this' gives the timer access to your AudioMgr object
// Note: This is outside of any function, just an example
// In actual code, place it inside a function like AudioMgr::startMyTimer()
// myTimer.start(500, [this]() { 
//     this->callbackAudioTestTimer(); 
// });

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional> // Needed for std::function

class VxTimedCallback {
public:
    VxTimedCallback();
    ~VxTimedCallback();

    // Set or change the callback function at any time
    void setCallback(std::function<void()> func);

    // Start with just 1 parameter (the interval)
    void start(int interval_ms);

    // Overload: Start and set callback at the same time (optional)
    void start(int interval_ms, std::function<void()> func);

    void stop();

    // Returns true if the timer thread is currently running
    bool isActive() const;


private:
    std::thread active_thread;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> stop_requested;
    
    // Track if the timer loop is alive and running
    std::atomic<bool> is_running; 

    // Stores the function pointer or lambda internally
    std::function<void()> callback_func; 
};

