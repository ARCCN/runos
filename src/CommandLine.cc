#include "CommandLine.hh"

#include <thread>
#include <iostream>
#include <utility> // pair
#include <cstdio>
#include <cctype> // isspace
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <QCoreApplication>
#include <histedit.h>

#include "Config.hh"

REGISTER_APPLICATION(CommandLine, {""})

static std::pair<
        std::string, // command name
        std::vector<std::string> // args
    > split(const char* line, size_t len)
{
    std::string str{line, len};
    std::istringstream ss{str};
    using str_it = std::istream_iterator<std::string>;
    std::string cmd_name;
    ss >> cmd_name;
    std::vector<std::string> args{str_it{ss}, str_it{}};
    std::pair<std::string, std::vector<std::string>> result {std::move(cmd_name),
                                                             std::move(args) };
    return result;
}

struct CommandLine::implementation {
    std::unique_ptr<EditLine, decltype(&el_end)> el
        { nullptr, &el_end };

    std::unique_ptr<History, decltype(&history_end)> hist
        { history_init(), &history_end };

    struct command_holder {
        cli::command fn;
        cli::options::options_description opts;
        std::string help_message;
    };

    std::unordered_map<std::string, command_holder> commands;

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
    register_builtins();
}

void CommandLine::register_builtins() {
    { // commands
    cli::options::options_description desc("Show existing commands");
    registerCommand("commands", std::move(desc), [=](auto vm) {
                for (const auto& p : m_impl->commands) {
                    std::cout << p.first << std::endl;
                }
            });
    } // commands
    { // help
        cli::options::options_description desc("Show help about command\n"
                                               "Usage : help <command>");
        desc.add_options()
            ("", cli::options::value<std::string>()->default_value("help"),
             "command name");
        registerCommand(
                "help",
                std::move(desc),
                [=](const cli::options::variables_map& vm) {
                    auto cmd = vm[""].as<std::string>();
                    try {
                        std::cout << m_impl->commands.at(cmd).opts;
                    }
                    catch (std::out_of_range& oo) {
                        std::cout << "Unknown command : " << cmd << std::endl <<
                                "Type \"commands\" for show existing commands";
                    }
            });
    } // help
}

void CommandLine::startUp(Loader*)
{
    std::thread {&implementation::run, m_impl.get()}.detach();
}

void CommandLine::registerCommand(
        cli::command_name&& spec,
        cli::options::options_description&& opts,
        cli::command&& fn
    )
{
    implementation::command_holder holder {std::move(fn), std::move(opts), ""};
    m_impl->commands.emplace(std::move(spec), std::move(holder));
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
    auto splitted = split(line, len);
    auto cmd_name = splitted.first;
    auto args = splitted.second;
    try {
        cli::options::variables_map vm;
        auto& cmd = commands.at(cmd_name);
        auto parsed =
            cli::options::command_line_parser(args).options(cmd.opts).run();
        cli::options::store(parsed, vm);
        cli::options::notify(vm);
        cmd.fn(vm);
    } catch (std::out_of_range& ex) {
        std::cout << "Unknown command: \"" << cmd_name << '"' << std::endl <<
            "Type \"commands\" for show existing commands";
    }
    catch (std::exception& ex) {
    }
    return true;
}

