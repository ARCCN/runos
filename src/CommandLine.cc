#include "CommandLine.hh"

#include <thread>
#include <iostream>
#include <utility> // pair
#include <cstdio>
#include <cctype> // isspace

#include <QCoreApplication>
#include <histedit.h>

#include "Config.hh"

REGISTER_APPLICATION(CommandLine, {""})

struct CommandLine::implementation {
    std::unique_ptr<EditLine, decltype(&el_end)> el
        { nullptr, &el_end };

    std::unique_ptr<History, decltype(&history_end)> hist
        { history_init(), &history_end };

    std::vector< std::pair<cli::pattern, cli::command> >
        commands;

    HistEvent hev;
    bool keep_reading { true };

    bool handle(const char* cmd, int len);
    void run();
};

CommandLine::CommandLine()
    : m_impl(new implementation)
{ }

CommandLine::~CommandLine() = default;

static const char* prompt(EditLine*)
{
    return "runos> ";
}

void CommandLine::init(Loader*, const Config& rootConfig)
{
    const auto& config = config_cd(rootConfig, "cli");

    history(m_impl->hist.get(), &m_impl->hev, H_SETSIZE,
            config_get(config, "history-size", 800));

    const char* argv0 = QCoreApplication::arguments().at(0).toLatin1().data();
    m_impl->el.reset(el_init(argv0, stdin, stdout, stderr));

    el_set(m_impl->el.get(), EL_PROMPT,
            &prompt);
    el_set(m_impl->el.get(), EL_EDITOR,
            config_get(config, "editor", "emacs").c_str());
    el_set(m_impl->el.get(), EL_HIST, history,
            m_impl->hist.get());
}

void CommandLine::startUp(Loader*)
{
    std::thread {&implementation::run, m_impl.get()}.detach();
}

void CommandLine::registerCommand(cli::pattern&& spec, cli::command&& fn)
{
    m_impl->commands.emplace_back(std::move(spec), std::move(fn));
}

void CommandLine::implementation::run()
{
    for (;keep_reading;) {
        int len;
        const char* line = el_gets(el.get(), &len);

        if (line == nullptr)
            break;

        if (handle(line, len)) {
            if (not std::isspace(*line)) {
                history(hist.get(), &hev, H_ENTER, line);
            }
        }
    }
}

bool CommandLine::implementation::handle(const char* line, int len)
{
    std::cout << line << std::endl;
    return true;
}

