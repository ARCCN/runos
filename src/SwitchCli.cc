#include "Switch.hh"

#include "Common.hh"
#include "CommandLine.hh"

#include <algorithm> // find

using namespace cli;

struct ShowSwitches {
    SwitchManager* app;
    ShowSwitches(SwitchManager* app) : app(app) { }
    void operator()(const options::variables_map& vm, Outside& out)
    {
        auto dpid = vm["dpid"];
        bool is_ports_needed = not vm["ports"].empty();

        if (not dpid.empty()) {
            auto sw = app->getSwitch(dpid.as<uint64_t>());
            print_switch_info(sw, out, is_ports_needed);
        } else {
            auto switches = app->switches();
            auto name_var = vm["name"];
            if (not name_var.defaulted()) {
                std::string name = name_var.as<std::string>();
                auto it = std::find_if(switches.begin(), switches.end(),
                        [&name](auto elem) { return elem->dp_desc() == name; });
                if (it != switches.end()) {
                    print_switch_info(*it, out, is_ports_needed);
                } else {
                    out.warning("Couldn't find switch with name {}", name);
                }
            } else {
                for (auto sw : switches) {
                    print_switch_info(sw, out, is_ports_needed);
                }
            }
        }
    }

    void print_switch_info(const Switch* sw, Outside& out, bool is_ports_needed)
    {
        if (sw == nullptr) {
            out.warning("Unreadable switch instances");
            LOG(ERROR) << "Nullptr switch";
        }
        out.print("Switch. Dpid                 : 0x{:x} \n"
                  "        Name                 : {}\n"
                  "        Table count          : {:d}\n"
                  "        Hardware description : {}\n"
                  "        Software description : {}\n"
                  "        Serial number        : {}\n",
                  sw->id(), sw->dp_desc(), sw->ntables(),
                  sw->hw_desc(), sw->sw_desc(), sw->serial_number());
        if (is_ports_needed) {
            print_ports_info(sw, out);
        }
    }

    void print_ports_info(const Switch* sw, Outside& out){
        auto ports = sw->ports();
        for (auto& p : ports) {
            out.print("       Port. Number : {:d}\n"
                      "             Name   : {}\n"
                      "             Hardware address : {}\n"
                      "             Current speed : {}\n"
                      "             Maximum speed : {}\n",
                      p.port_no(), p.name(), p.hw_addr().to_string(),
                      p.curr_speed(), p.max_speed());
        }
    }

    options::options_description get_descriptions() const {
        options::options_description desc;
        desc.add_options()
            ("dpid,d", options::value<uint64_t>(),
             "Dpid of switch, info about should be printed")
            ("name,n", options::value<std::string>()->default_value("all"),
             "Name of switch, info about should be printed")
            ("ports,p", "Show ports of switch");
        return desc;
    }
};

class SwitchCli : public Application {
SIMPLE_APPLICATION(SwitchCli, "switch-manager-cli")
public:
    void init(Loader* loader, const Config& config) override
    {
        auto app = SwitchManager::get(loader);
        auto cli = CommandLine::get(loader);
        ShowSwitches show_switches{app};
        auto desc = show_switches.get_descriptions();
        cli->registerCommand("switches", std::move(desc), std::move(show_switches), "Print switches info");
    }
private:
};

REGISTER_APPLICATION(SwitchCli,
        {"switch-manager", "command-line-interface", ""})
