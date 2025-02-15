#include <iostream>
#include "./../ext/headers/args.hxx"
#include "cpu.h"

using namespace dramsim3;

int main(int argc, const char **argv) {
    args::ArgumentParser parser(
        "DRAM Simulator.",
        "Examples: \n."
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100 -t "
        "sample_trace.txt\n"
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -s random -c 100");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<uint64_t> num_cycles_arg(parser, "num_cycles",
                                             "Number of cycles to simulate",
                                             {'c', "cycles"}, 100000);
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> stream_arg(
        parser, "stream_type", "address stream generator - (random), stream",
        {'s', "stream"}, "");
    args::ValueFlag<std::string> trace_file_arg(
        parser, "trace",
        "Trace file, setting this option will ignore -s option",
        {'t', "trace"});
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");
    args::Flag is_all_trans_mode(parser, "is_all_trans_mode", "is_all_trans_mode", {'a', "all_trans"});
    // args::ValueFlag<uint64_t> is_all_trans_mode(parser, "is_all_trans_mode",
    //                                          "whether to use all transaction mode",
    //                                          {'a', "all_trans"}, 0);

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(num_cycles_arg);
    std::string output_dir = args::get(output_dir_arg);
    std::string trace_file = args::get(trace_file_arg);
    std::string stream_type = args::get(stream_arg);

    CPU *cpu;
    if (!trace_file.empty()) {
        cpu = new TraceBasedCPU(config_file, output_dir, trace_file);
    } else {
        if (stream_type == "stream" || stream_type == "s") {
            cpu = new StreamCPU(config_file, output_dir);
        } else {
            cpu = new RandomCPU(config_file, output_dir);
        }
    }

    bool sim_end = false;
    bool IsAllTransFinished=false;
    std::vector<bool> CmdQueueEmpty;
    std::vector<bool> PendingWrQEmpty;
    std::vector<bool> PendingRdQEmpty;
    std::vector<bool> UnifiedQueueEmpty;
    std::vector<bool> WriteBufferEmpty;
    std::vector<bool> ReadQueueEmpty;
    std::vector<bool> ReturnQueueEmpty;

    for(int i=0; i<cpu->getMemorySystem().getDRAMSystem()->total_channels_; i++) {
      CmdQueueEmpty.push_back(false);
      PendingWrQEmpty.push_back(false);
      PendingRdQEmpty.push_back(false);
      UnifiedQueueEmpty.push_back(false);
      WriteBufferEmpty.push_back(false);
      ReadQueueEmpty.push_back(false);
      ReturnQueueEmpty.push_back(false);
    }

    if(args::get(is_all_trans_mode) == 0) {
      for (uint64_t clk = 0; clk < cycles; clk++) {
          cpu->ClockTick();
      }
    } else {
      while(!(sim_end)) {
        cpu->ClockTick();

        // check queue empty for controllers
        IsAllTransFinished = cpu->AllTransactionsFinished();

        for(int i = 0; i < cpu->getMemorySystem().getDRAMSystem()->GetControllers().size(); i++) {
          CmdQueueEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetCommandQueue().QueueEmpty();
          PendingWrQEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetPendingWriteQueue().empty();
          PendingRdQEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetPendingReadQueue().empty();
          UnifiedQueueEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetUnifiedQueue().empty();
          WriteBufferEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetWriteBuffer().empty();
          ReadQueueEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetReadQueue().empty();
          ReturnQueueEmpty[i] = cpu->getMemorySystem().getDRAMSystem()->GetController(i)->GetReturnQueue().empty();

          sim_end = IsAllTransFinished & CmdQueueEmpty[i] & PendingWrQEmpty[i] & PendingRdQEmpty[i] & UnifiedQueueEmpty[i] & WriteBufferEmpty[i] & ReadQueueEmpty[i];
        }
      }
    }

    cpu->PrintStats();

    delete cpu;

    return 0;
}
