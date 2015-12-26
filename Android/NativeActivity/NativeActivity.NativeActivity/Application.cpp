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
        updateInvoker();

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
    AInputEvent* event = nullptr;
    while (AInputQueue_getEvent(m_inputQueue, &event) >= 0)
    {
        //LOGV("New input event: type=%d\n", AInputEvent_getType(event));
        if (AInputQueue_preDispatchEvent(m_inputQueue, event))
        {
            continue;
        }

        boost::optional<bool> handled = m_onInputSignal(event);

        AInputQueue_finishEvent(m_inputQueue, event, handled ? 1 : 0);
    }
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
    if (needInvoke())
    {
        performCrossThreadCall(std::bind(&Application::setState, this, state), this, true);
    }
    else
    {
        m_activityState.store(state, std::memory_order_relaxed);
    }
}

void Application::setInput(AInputQueue* input)
{
    if (needInvoke())
    {
        performCrossThreadCall(std::bind(&Application::setInput, this, input), this, true);
    }
    else
    {
        if (m_inputQueue)
        {
            AInputQueue_detachLooper(m_inputQueue);
        }

        m_inputQueue = input;

        if (m_inputQueue)
        {
            AInputQueue_attachLooper(m_inputQueue,
                m_looper, LooperIdInput, nullptr, nullptr);
        }
    }
}

void Application::setWindow(ANativeWindow* window)
{
    if (needInvoke())
    {
        performCrossThreadCall(std::bind(&Application::setWindow, this, window), this, true);
    }
    else
    {
        m_window = window;

        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        eglInitialize(display, nullptr, nullptr);

        const EGLint attribs[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
        };

        EGLint w, h, format;
        EGLint numConfigs;
        EGLConfig config;
        EGLSurface surface;
        EGLContext context;

        eglChooseConfig(display, attribs, &config, 1, &numConfigs);
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

        ANativeWindow_setBuffersGeometry(m_window, 0, 0, format);

        surface = eglCreateWindowSurface(display, config, m_window, nullptr);

        const EGLint contextAttribs[] = 
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        context = eglCreateContext(display, config, nullptr, contextAttribs);

        if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        {
            //LOGW("Unable to eglMakeCurrent");
            return;
        }
    }
}

void Application::onStart(ANativeActivity* act)
{
    Application::getRef().setState(StateStart);
}

void Application::onInputCreated(ANativeActivity* act, AInputQueue* input)
{
    Application::getRef().setInput(input);
}

void Application::onInputDestroyed(ANativeActivity* act, AInputQueue* input)
{
    Application::getRef().setInput(nullptr);
}

void Application::onWindowCreated(ANativeActivity* act, ANativeWindow* window)
{
    Application::getRef().setWindow(window);
}

FSignals::connection Application::attachOnInputSignal(const OnInputSignal_t::slot_type& slot)
{
    return m_onInputSignal.connect(slot);
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
// 
//     activity->instance = android_app_create(activity, savedState, savedStateSize);

    Application* app = new Application(activity);

    if (app->initialize())
    {
        activity->instance = app;

        activity->callbacks->onStart = Application::onStart;
        activity->callbacks->onInputQueueCreated = Application::onInputCreated;
        activity->callbacks->onInputQueueDestroyed = Application::onInputDestroyed;
        activity->callbacks->onNativeWindowCreated = Application::onWindowCreated;
    }
}

#endif