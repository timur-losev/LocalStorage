#include "Application.h"
#include "Core/FDateTime.h"

SINGLETON_SETUP(Application);


Application::Application(ANativeActivity* act) : m_activity(act), m_isRunning(false)
{
    SINGLETON_ENABLE_THIS;
}

Application::~Application()
{

}

void Application::appEntry()
{
    m_looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

    ALooper_addFd(m_looper,
        m_pipes[PP_READ],
        LooperIdMain,
        ALOOPER_EVENT_INPUT,
        nullptr, nullptr);

    processHandlers.resize(2);

    processHandlers[LooperIdMain - 1] = std::bind(&Application::processCmd, this);
    processHandlers[LooperIdInput - 1] = std::bind(&Application::processInput, this);

    m_isRunning = true;

    createInvoker();

    m_onAppInitializedVar.notify_all();

    while (true)
    {


        int ident, events;

        while ((ident = ALooper_pollAll(0, nullptr, &events, nullptr)) >= 0)
        {
            processHandlers[ident - 1]();
        }


    }
}

void Application::processCmd()
{

}

void Application::processInput()
{

}

bool Application::initialize()
{
    if (pipe(m_pipes))
    {
        FLogCritical(("could not create pipe: %1%") % strerror(errno));
        return false;
    }

    m_worker = FMakeShared<FThread_t>(std::bind(&Application::appEntry, this));

    FUniqueLock_t lock(m_mutex);
    m_onAppInitializedVar.wait(lock);

    return true;
}

void Application::destroy()
{

}

void Application::setState(ActivityState state)
{
    m_activityState.store(state, std::memory_order_relaxed);
}

void Application::onStart(ANativeActivity* act)
{
    Application::getRef().setState(StateStart);
}

void Application::onInputCreated(AInputQueue* input)
{

}

void Application::writeCmd(int8_t cmd)
{
    if (write(m_pipes[PP_WRITE], &cmd, sizeof(cmd)) != sizeof(cmd))
    {
        FAssert(false);
    }
}

#if USE_APP
void ANativeActivity_onCreate(ANativeActivity* activity,
    void* savedState, size_t savedStateSize) {
    
    new FLogger();

//     activity->callbacks->onDestroy = onDestroy;
//     activity->callbacks->onStart = onStart;
//     activity->callbacks->onResume = onResume;
//     activity->callbacks->onSaveInstanceState = onSaveInstanceState;
//     activity->callbacks->onPause = onPause;
//     activity->callbacks->onStop = onStop;
//     activity->callbacks->onConfigurationChanged = onConfigurationChanged;
//     activity->callbacks->onLowMemory = onLowMemory;
//     activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
//     activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
//     activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
//     activity->callbacks->onInputQueueCreated = onInputQueueCreated;
//     activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
// 
//     activity->instance = android_app_create(activity, savedState, savedStateSize);

    Application* app = new Application(activity);

    if (app->initialize())
    {
        activity->instance = app;

        activity->callbacks->onStart = Application::onStart;
    }
}

#endif