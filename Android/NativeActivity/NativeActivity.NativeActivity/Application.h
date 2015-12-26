#pragma once

#include <poll.h>
#include <pthread.h>
#include <sched.h>

#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>

#include "FLazySingleton.h"

#include "Core/AsyncOperation/FCrossThreadInvoker.h"

class Application : public FLazySingleton<Application>, public FCrossThreadInvoker
{
public:

    enum ActivityState
    {
        /**
        * Command from main thread: the AInputQueue has changed.  Upon processing
        * this command, android_app->inputQueue will be updated to the new queue
        * (or NULL).
        */
        StateInputChanged,

        /**
        * Command from main thread: a new ANativeWindow is ready for use.  Upon
        * receiving this command, android_app->window will contain the new window
        * surface.
        */
        StateInitWindow,

        /**
        * Command from main thread: the existing ANativeWindow needs to be
        * terminated.  Upon receiving this command, android_app->window still
        * contains the existing window; after calling android_app_exec_cmd
        * it will be set to NULL.
        */
        StateTermWindow,

        /**
        * Command from main thread: the current ANativeWindow has been resized.
        * Please redraw with its new size.
        */
        StateWindowResized,

        /**
        * Command from main thread: the system needs that the current ANativeWindow
        * be redrawn.  You should redraw the window before handing this to
        * android_app_exec_cmd() in order to avoid transient drawing glitches.
        */
        StateWindowRedrawNeeded,

        /**
        * Command from main thread: the content area of the window has changed,
        * such as from the soft input window being shown or hidden.  You can
        * find the new content rect in android_app::contentRect.
        */
        StateContentRectChanged,


        /**
        * Command from main thread: the app's activity window has gained
        * input focus.
        */
        StateGainedFocus,

        /**
        * Command from main thread: the app's activity window has lost
        * input focus.
        */
        StateLostFocus,

        /**
        * Command from main thread: the current device configuration has changed.
        */
        StateConfigChanged,

        /**
        * Command from main thread: the system is running low on memory.
        * Try to reduce your memory use.
        */
        StateLowMemory,

        /**
        * Command from main thread: the app's activity has been started.
        */
        StateStart,

        /**
        * Command from main thread: the app's activity has been resumed.
        */
        StateResume,

        /**
        * Command from main thread: the app should generate a new saved state
        * for itself, to restore from later if needed.  If you have saved state,
        * allocate it with malloc and place it in android_app.savedState with
        * the size in android_app.savedStateSize.  The will be freed for you
        * later.
        */
        StateSave,

        /**
        * Command from main thread: the app's activity has been paused.
        */
        StatePause,

        /**
        * Command from main thread: the app's activity has been stopped.
        */
        StateStop,

        /**
        * Command from main thread: the app's activity is being destroyed,
        * and waiting for the app thread to clean up and exit before proceeding.
        */
        StateDestroy
    };

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

    typedef FSignals::signal<bool(AInputEvent*)> OnInputSignal_t;
private:
    ANativeActivity*    m_activity          = nullptr;
    ALooper*            m_looper            = nullptr;
    AInputQueue*        m_inputQueue        = nullptr;
    ANativeWindow*      m_window            = nullptr;
    std::atomic_int     m_activityState;

    FConditionVariable_t m_onAppInitializedVar;
    FMutex_t             m_mutex;

    FSharedPtr<FThread_t> m_worker;
    OnInputSignal_t m_onInputSignal;

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

    void setState(ActivityState state);
    void setInput(AInputQueue* input);
    void setWindow(ANativeWindow* window);

    FArray<std::function<void()>> processHandlers;

    static void onStart(ANativeActivity* act);
    static void onInputCreated(ANativeActivity* act, AInputQueue* input);
    static void onInputDestroyed(ANativeActivity* act, AInputQueue* input);
    static void onWindowCreated(ANativeActivity* act, ANativeWindow* window);

    FSignals::connection attachOnInputSignal(const OnInputSignal_t::slot_type& slot);

    void writeCmd(int8_t cmd);
};