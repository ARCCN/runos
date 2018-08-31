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

std::vector<std::string> split(const char* line, size_t len)
{
    std::string str{line, len};
    std::istringstream ss{str};
    using str_it = std::istream_iterator<std::string>;
    std::vector<std::string> args{str_it{ss}, str_it{}};
    return args;
}

struct CommandLine::implementation {
    std::unique_ptr<EditLine, decltype(&el_end)> el
        { nullptr, &el_end };

    std::unique_ptr<History, decltype(&history_end)> hist
        { history_init(), &history_end };

    struct command_holder {
        cli::command fn;
        cli::options::options_description opts;
        cli::options::positional_options_description pos_opts;
        const char* help;
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
    registerCommand("commands", [=](auto vm) {
                std::vector<std::string> commands_name;
                commands_name.reserve(m_impl->commands.size());
                for (const auto& p : m_impl->commands) {
                    commands_name.push_back(p.first);
                }
                std::sort(commands_name.begin(), commands_name.end());
                for (const auto& name : commands_name) {
                    std::cout << name << std::endl;
                }
            }, "List of commands");
    } // commands

    { // help
        cli::options::options_description desc;
        desc.add_options()
             ("command-name", cli::options::value<std::string>(), "name of command");
        cli::options::positional_options_description pos;
        pos.add("command-name", 1)
           .add("others", -1);
        auto help_fn =
            [=](const cli::options::variables_map& vm) {
                auto arg = vm["command-name"];
                std::string cmd_name =
                    arg.empty() ? "help" : arg.as<std::string>();
                try {
                    std::cout << cmd_name  << ":" << std::endl << std::endl;
                    const auto& cmd = m_impl->commands.at(cmd_name);
                    std::cout << cmd.help;
                    std::cout << std::endl;
                    std::cout << cmd.opts;
                    std::cout << std::endl;
                }
                catch (std::out_of_range& oor) {
                    std::cout << "Unknown command : " << cmd_name << std::endl <<
                            "Type \"commands\" for show existing commands"
                            << std::endl;
                }
            };
        registerCommand("help", std::move(desc), std::move(pos), std::move(help_fn),
                "Help!!! I need somebody\n"
                "Print help of command\n"
                "Usage: help <command>\n"
            );
    } // help

    registerCommand("whoserules", [=](auto vm) {

		const char* whoserules =
"                                                    \n"
"                   `-.        .-'.                  \n"
"                `-.    -./\\.-    .-'                \n"
"                    -.  /_|\\  .-                    \n"
"                `-.   `/____\\'   .-'.               \n"
"             `-.    -./.-\"\"-.\\.-      '             \n"
"                `-.  /<  (O) >\\  .-'                \n"
"              -   .`/__`-..-'__\\'   .-              \n"
"            ,...`-./___|____|___\\.-'.,.             \n"
"               ,-'   ,` . . ',   `-,                \n"
"            ,-'   ________________  `-,             \n"
"               ,'/____|_____|_____\\                 \n"
"              / /__|_____|_____|___\\                \n"
"             / /|_____|_____|_____|_\\               \n"
"            ' /____|_____|_____|_____\\              \n"
"          .' /__|_____|_____|_____|___\\             \n"
"         ,' /|_____|_____|_____|_____|_\\            \n"
"        /../____|_____|_____|_____|_____\\           \n"
"       '../__|_____|_____|_____|_____|___\\          \n"
"      '.:/|_____|_____|_____|_____|_____|_\\         \n"
"    ,':./____|_____|_____|_____|_____|_____\\         \n"
"   /:../__|_____|_____|_____|_____|_____|___\\         \n"
"  /.../|_____|_____|_____|_____|_____|_____|_\\      \n"
" /'.:/____|_____|_____|_____|_____|_____|_____\\  \n"
"(\\:./ _  _ ___  ____ ____ _    _ _ _ _ _  _ ___\\  \n"
" \\./              MY RUNOS MY RULES             \\ \n"
"   \"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"                     \n";
			std::cout << whoserules;
}, "");
}

void CommandLine::startUp(Loader*)
{
    std::thread {&implementation::run, m_impl.get()}.detach();
}

void CommandLine::registerCommand(
        cli::command_name&& spec,
        cli::command&& fn,
        const char* help
    )
{
    registerCommand(
            std::move(spec),
            cli::options::options_description{}, // empty description
            cli::options::positional_options_description{}, // empty description
            std::move(fn),
            help
        );
}

void CommandLine::registerCommand(
        cli::command_name&& spec,
        cli::options::options_description&& opts,
        cli::command&& fn,
        const char* help
    )
{
    registerCommand(
            std::move(spec),
            std::move(opts),
            cli::options::positional_options_description{}, // empty description
            std::move(fn),
            help
        );
}

void CommandLine::registerCommand(
        cli::command_name&& spec,
        cli::options::options_description&& opts,
        cli::options::positional_options_description&& pos_opts,
        cli::command&& fn,
        const char* help
    )
{
    implementation::command_holder holder{
            std::move(fn),
            std::move(opts),
            std::move(pos_opts),
            help
        };
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
    auto argv = split(line, len);
    if (not argv.empty()) try {
        cli::options::variables_map vm;
        auto& cmd = commands.at(argv[0]);
        argv.erase(argv.begin());
        auto parsed =
            cli::options::command_line_parser(argv).
            options(cmd.opts).
            positional(cmd.pos_opts).
            allow_unregistered().
            run();
        cli::options::store(parsed, vm);
        cli::options::notify(vm);
        cmd.fn(vm);
    } catch (std::out_of_range& ex) {
        std::cout << "Unknown command: \"" << argv[0] << '"' << std::endl <<
            "Type \"commands\" for show existing commands" << std::endl;
    }
    catch (std::exception& ex) {
        std::cout << "Unknown exception : " << ex.what() << std::endl;
    }
    return true;
}

