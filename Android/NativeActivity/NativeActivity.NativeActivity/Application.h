#pragma once

#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>

#include "FLazySingleton.h"

class Application : public FLazySingleton<Application>
{
public:
    enum Loppers
    {
        /**
        * Looper data ID of commands coming from the app's main thread, which
        * is returned as an identifier from ALooper_pollOnce().  The data for this
        * identifier is a pointer to an android_poll_source structure.
        * These can be retrieved and processed with android_app_read_cmd()
        * and android_app_exec_cmd().
        */
        LooperIdMain = 1,

        /**
        * Looper data ID of events coming from the AInputQueue of the
        * application's window, which is returned as an identifier from
        * ALooper_pollOnce().  The data for this identifier is a pointer to an
        * android_poll_source structure.  These can be read via the inputQueue
        * object of android_app.
        */
        LooperIdInput,

        /**
        * Start of user-defined ALooper identifiers.
        */
        LopperIdUser
    };
private:
    ANativeActivity*    m_activity = nullptr;
    ALooper*            m_looper = nullptr;

    FSharedPtr<FThread_t> m_worker;

    enum Pipe
    {
        PP_READ, PP_WRITE
    };

    int m_pipes[2];

    
    void appEntry();

    void processCmd();
    void processInput();

public:

    Application(ANativeActivity*);
    ~Application();

    bool initialize();
    void destroy();

    std::atomic_bool m_isRunning;

    FArray<std::function<void()>> processHandlers;

    static void onStart(ANativeActivity* act);
};