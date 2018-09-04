#include "CommandLine.hh"

#include <thread>
#include <iostream>
#include <utility> // pair
#include <cstdio>
#include <cctype> // isspace
#include <unordered_map>
#include <initializer_list>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/info.hpp>

#include <QCoreApplication>
#include <histedit.h>

#include "Config.hh"
#include "types/exception.hh"

enum class settings : uint8_t {
    black           =  30,
    red             =  31,
    green           =  32,
    yellow          =  33,
    blue            =  34,
    magenta         =  35,
    cyan            =  36,
    white           =  37,
    reset           =   0,
    bold_bright     =   1,
    underline       =   4,
    inverse         =   7,
    bold_bright_off =  21,
    underline_off   =  24,
    inverse_off     =  27,
    blink           =   5,
};

class setup {
public:
    setup(std::initializer_list<settings> setts)
        : m_out(nullptr), m_settings(setts)
    { }
    ~setup() {
        if (m_out) {
            *m_out << "\033[0m";
        }
    }

    struct double_setup_error : virtual runos::runtime_error { };

    friend std::ostream& operator<<(std::ostream& out, setup&& s)
    {
        if (s.m_out != nullptr) {
            RUNOS_THROW(double_setup_error()
                    << runos::errinfo_msg("Try setup setting second time"));
        }
        s.m_out = &out;
        out << "\033["; // start of manage commands
        for (auto i = s.m_settings.begin(); i != s.m_settings.end(); i++) {
            out << static_cast<uint32_t>(*i);
            // separate or end commands
            out << (i != s.m_settings.end() - 1 ? ';' : 'm');
        }
        return out;
    }
private:
    std::ostream* m_out;
    std::vector<settings> m_settings;
};


REGISTER_APPLICATION(CommandLine, {""})

std::vector<std::string> split(const char* line, size_t len)
{
    std::string str{line, len};
    std::istringstream ss{str};
    using str_it = std::istream_iterator<std::string>;
    std::vector<std::string> args{str_it{ss}, str_it{}};
    return args;
}

class cli::Outside::Backend {
public:
    virtual ~Backend() = default;
    virtual void print(const std::string& msg) = 0;
    virtual void echo(const std::string& msg) = 0;
    virtual void warning(const std::string& msg) = 0;
    virtual void error(const std::string& msg) = 0;
};

class Stdout : public cli::Outside::Backend {
    void print(const std::string& msg) override
    { std::cout << msg << std::endl; }

    void echo(const std::string& msg) override
    { std::cout << msg; }

    void warning(const std::string& msg) override
    {
        std::cout << setup{settings::yellow} << "Warning: ";
        std::cout << msg << std::endl;
    }

    void error(const std::string& msg) override
    {
        RUNOS_THROW( cli::error() << runos::errinfo_str(msg) );
    }
};

template<typename ...Args>
void cli::Outside::print(fmt::string_view format_str, const Args&... args)
{
    // fmt::make_format_args is not working. I hope just `args...` deal with
    // forwarding argiments into format.
    m_backend.print(fmt::format(format_str, args...));
}

template<typename ...Args>
void cli::Outside::echo(fmt::string_view format_str, const Args&... args)
{
    m_backend.echo(fmt::format(format_str, args...));
}

template<typename ...Args>
void cli::Outside::warning(fmt::string_view format_str, const Args&... args)
{
    m_backend.warning(fmt::format(format_str, args...));
}

template<typename ...Args>
void cli::Outside::error(fmt::string_view format_str, const Args&... args)
{
    m_backend.error(fmt::format(format_str, args...));
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
    registerCommand("commands", [=](auto vm, cli::Outside& out) {
                std::vector<std::string> commands_name;
                commands_name.reserve(m_impl->commands.size());
                for (const auto& p : m_impl->commands) {
                    commands_name.push_back(p.first);
                }
                std::sort(commands_name.begin(), commands_name.end());
                for (const auto& name : commands_name) {
                    out.print("{}", name);
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
            [=](const cli::options::variables_map& vm, cli::Outside& out) {
                auto arg = vm["command-name"];
                std::string cmd_name =
                    arg.empty() ? "help" : arg.as<std::string>();
                try {
                    std::cout << cmd_name  << ":" << std::endl << std::endl;
                    const auto& cmd = m_impl->commands.at(cmd_name);
                    out.print("{}", cmd.help);
                    out.print("{}", boost::lexical_cast<std::string>(cmd.opts));
                }
                catch (std::out_of_range& oor) {
                    // TODO: move it in Outside class
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

    registerCommand("whoserules", [=](auto vm, cli::Outside& out) {

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
            out.print(whoserules);
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
    cli::options::variables_map vm;
    if (not argv.empty()) try {
        auto& cmd = commands.at(argv[0]);
        argv.erase(argv.begin());
        auto parsed =
            cli::options::command_line_parser(argv).
            options(cmd.opts).
            positional(cmd.pos_opts).
            allow_unregistered().
            run();
        cli::options::store(parsed, vm);
        Stdout stream;
        cli::Outside out(stream);
        cli::options::notify(vm);
        cmd.fn(vm, out);
    } catch (std::out_of_range& ex) {
        std::cout << setup{settings::yellow} <<
            "Unknown command: \"" << argv[0] << '"' << std::endl <<
            "Type \"commands\" for show existing commands" << std::endl;
    } catch (cli::error& ex) {
        std::cout << setup{settings::red} << "Error: " << std::endl;
        std::cout << setup{settings::yellow} << ex.what();
    } catch (std::exception& ex) {
        std::cout << setup{settings::red} <<
            "Unknown error : " << ex.what() << std::endl;
    } catch (...) {
        std::cout << setup{settings::red} <<
            "Unknown error";
    }
    return true;
}

