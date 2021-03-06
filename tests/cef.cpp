#ifndef EMSCRIPTEN
#include "cef.h"

#include "include/cef_client.h"
#include "embindcefv8.h"
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

CefRefPtr<Handler>
    handler;
CefRefPtr<CefBrowser>
    browser;
bool
    mustProcess(true);
std::vector<std::string>
    filesToExecute;

std::string getFileContent(const std::string & name)
{
    std::ifstream
        i(name);
    std::string
        str((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());

    return str;
}

class StopHandler : public CefV8Handler {
public:
    StopHandler() {}

    virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) OVERRIDE
    {
        mustProcess = false;
        return true;
    }

    IMPLEMENT_REFCOUNTING(LocalV8Handler);
};

class RequireHandler : public CefV8Handler {
public:
    RequireHandler() {}

    virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) OVERRIDE
    {
        return true;
    }

    IMPLEMENT_REFCOUNTING(LocalV8Handler);
};

class LogHandler : public CefV8Handler {
public:
    LogHandler() {}

    virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) OVERRIDE
    {
        for(size_t i=0; i<arguments.size(); ++i)
        {
            auto & argument = * arguments[i];

            if(argument.IsString())
            {
                std::cout << argument.GetStringValue().ToString();
            }
            else if(argument.IsInt())
            {
                std::cout << argument.GetIntValue();
            }
            else if(argument.IsDouble())
            {
                std::cout << argument.GetDoubleValue();
            }
        }

        std::cout << std::endl;

        return true;
    }

    IMPLEMENT_REFCOUNTING(LocalV8Handler);
};

void App::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    CefRefPtr<CefV8Value> stop_func = CefV8Value::CreateFunction("stop", new StopHandler());
    CefRefPtr<CefV8Value> require_func = CefV8Value::CreateFunction("require", new RequireHandler());
    CefRefPtr<CefV8Value> log_func = CefV8Value::CreateFunction("log", new LogHandler());

    context->GetGlobal()->SetValue("stop", stop_func, V8_PROPERTY_ATTRIBUTE_NONE);
    context->GetGlobal()->SetValue("require", require_func, V8_PROPERTY_ATTRIBUTE_NONE);
    context->GetGlobal()->GetValue("console")->SetValue("log", log_func, V8_PROPERTY_ATTRIBUTE_NONE);

    embindcefv8::onContextCreated(& * context);
}

void App::OnContextInitialized()
{
    CefWindowInfo window_info;

    window_info.SetAsOffScreen(nullptr);
    window_info.SetTransparentPainting(true);

    CefBrowserSettings browser_settings;
    browser = CefBrowserHost::CreateBrowserSync(window_info, & * handler, "about:blank", browser_settings, nullptr);

    std::string content = "";

    for(auto f : filesToExecute)
    {
        content += "<script>";
        content += getFileContent(f);
        content += "</script>";
    }

    content += "<script>stop();</script>";

    browser->GetMainFrame()->LoadString(content, "dummy");
    embindcefv8::setBrowser(browser);
}

void App::OnRegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar)
{
}

bool Handler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
    rect = CefRect(0, 0, 128, 128);
    return true;
}

void Handler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
{
}

bool Handler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString& message, const CefString& source, int line)
{
    puts(message.ToString().c_str());
    return true;
}

void initCef(int argc, char *argv[])
{
    #ifdef _LINUX
        CefMainArgs args(argc, argv);
    #elif defined(_WINDOWS)
        CefMainArgs args(GetModuleHandle(NULL));
    #endif

    CefRefPtr<App> app(new App);

    int exit_code = CefExecuteProcess(args, app.get(), nullptr);

    if (exit_code >= 0)
    {
        exit(exit_code);
        return;
    }

    handler = new Handler();

    CefSettings settings;
    memset(&settings, 0, sizeof(CefSettings));
    settings.single_process = true;
    settings.no_sandbox = true;
    settings.multi_threaded_message_loop = false;
    settings.log_severity = LOGSEVERITY_DISABLE;
    settings.size = sizeof(CefSettings);

    CefInitialize(args, settings, app.get(), nullptr);
}

void processLoop()
{
    mustProcess = true;

    while(mustProcess)
    {
        CefDoMessageLoopWork();
    }
}

void finalizeCef()
{
    CefShutdown();
}

void executeFile(const char *src)
{
    filesToExecute.push_back(src);
}

#endif
