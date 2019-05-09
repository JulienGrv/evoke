#include "Configuration.h"
#include "Executor.h"
#include "Project.h"
#include "Reporter.h"
#include "Toolset.h"

#include <boost/program_options.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <thread>

using namespace std::literals::chrono_literals;

namespace po = boost::program_options;

template<typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> v)
{
    os << "[\n";
    bool first = true;
    for(auto &e : v)
    {
        if(first)
            first = false;
        else
            os << ", ";
        os << *e << "\n";
    }
    os << "]\n";
    return os;
}

int main(int argc, const char **argv)
{
    // Init arguments
    bool help = false;
    bool verbose = false;
    std::string root_path = filesystem::current_path().generic_string();
    unsigned int job_count = std::max(4u, std::thread::hardware_concurrency());
#if defined(_WIN32)
    std::string toolset_name = "windows";
#else
    std::string toolset_name = "gcc";
#endif
    std::string reporter_name = "guess";
    bool compilation_database = false;
    bool cmakelists = false;
    bool unity_build = false;

    // Init arguments parser
    po::options_description desc("Options");
    desc.add_options()("help,h", po::bool_switch(&help), "print this message")("verbose", po::bool_switch(&verbose), "enable verbose output")("root", po::value(&root_path), "root directory path of the project")("jobs,j", po::value(&job_count), "build in parallel using N jobs")("toolset,t", po::value(&toolset_name), "toolset name")("reporter,r", po::value(&reporter_name), "reporter name")("cp", po::bool_switch(&compilation_database), "generate compile_commands.json")("cm", po::bool_switch(&cmakelists), "generate CMakelists.txt")("unity,u", po::bool_switch(&unity_build), "create commands for Unity");

    // Parse arguments
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Handle arguments
    if(help)
    {
        std::cout << desc << "\n";
    }
    else
    {
        Project op(root_path);
        if(!op.unknownHeaders.empty())
        {
            /*
      // TODO: allow building without package fetching somehow
      std::string fetch = "accio fetch";
      std::vector<std::string> hdrsToFetch(op.unknownHeaders.begin(), op.unknownHeaders.end());
      for (auto& hdr : hdrsToFetch) fetch += " " + hdr;
      system(fetch.c_str());
      op.Reload();
    */
        }
        for(auto &u : op.unknownHeaders)
        {
            std::cerr << "Unknown header: " << u << "\n";
        }

        std::cout << "Building for " << toolset_name << "\n";
        std::unique_ptr<Toolset> toolset = GetToolsetByName(toolset_name);

        if(unity_build)
        {
            toolset->CreateCommandsForUnity(op);
        }
        else
        {
            toolset->CreateCommandsFor(op);
        }

        if(compilation_database)
        {
            std::ofstream os("compile_commands.json");
            op.dumpJsonCompileDb(os);
        }

        if(cmakelists)
        {
            auto opts = toolset->ParseGeneralOptions(Configuration::Get().compileFlags);
            op.dumpCMakeListsTxt(opts);
        }

        if(verbose)
        {
            std::cout << op;
        }

        std::unique_ptr<Reporter> reporter = Reporter::Get(reporter_name);

        Executor ex(job_count, *reporter);
        for(auto &comp : op.components)
        {
            for(auto &c : comp.second.commands)
            {
                if(c->state == PendingCommand::ToBeRun)
                    ex.Run(c);
            }
        }
        ex.Start().get();

        printf("\n\n");
    }

    return 0;
}
