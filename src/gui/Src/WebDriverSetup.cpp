#include "WebDriverSetup.h"

#include <QWidget>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "webdriver_server.h"
#include "webdriver_view_transitions.h"
#include "webdriver_route_table.h"
#include "webdriver_switches.h"
#include "extension_qt/q_view_runner.h"
#include "extension_qt/q_session_lifecycle_actions.h"
#include "extension_qt/widget_view_enumerator.h"
#include "extension_qt/widget_view_executor.h"

static base::AtExitManager* g_exitManager = nullptr;

int startWebDriver(int argc, char* argv[])
{
    g_exitManager = new base::AtExitManager();

    webdriver::ViewRunner::RegisterCustomRunner<webdriver::QViewRunner>();
    webdriver::SessionLifeCycleActions::RegisterCustomLifeCycleActions<webdriver::QSessionLifeCycleActions>();
    webdriver::ViewTransitionManager::SetURLTransitionAction(new webdriver::URLTransitionAction_CloseOldView());

    // Only register enumerator and executor (no creator) so sessions must
    // attach to existing windows via browserStartWindow capability instead
    // of spawning blank QWidget windows.
    webdriver::ViewEnumerator::AddViewEnumeratorImpl(new webdriver::WidgetViewEnumeratorImpl());
    webdriver::ViewCmdExecutorFactory::GetInstance()->AddViewCmdExecutorCreator(new webdriver::QWidgetViewCmdExecutorCreator());

    CommandLine cmd_line(CommandLine::NO_PROGRAM);
    for(int i = 0; i < argc; ++i)
        cmd_line.AppendSwitch(argv[i]);

    // Bind to localhost only for security
    if(!cmd_line.HasSwitch(webdriver::Switches::kPort))
        cmd_line.AppendSwitchASCII(webdriver::Switches::kPort, "127.0.0.1:9517");

    auto* wd_server = webdriver::Server::GetInstance();
    if(0 != wd_server->Configure(cmd_line))
        return 1;

    return wd_server->Start();
}

void stopWebDriver()
{
    webdriver::Server::GetInstance()->Stop();
    delete g_exitManager;
    g_exitManager = nullptr;
}
