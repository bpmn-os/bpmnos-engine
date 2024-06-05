#include <iostream>
#include <vector>
#include <string>
#include <bpmn++.h>
#include <bpmnos-model.h>
#include <bpmnos-execution.h>

void print_usage() {
  std::cout << "Usage:" << std::endl;
  std::cout << "\tbpmnos --model <model file> --data <data file> [--paths <path1> <path2> ...] [--verbose]" << std::endl;
  std::cout << "\tbpmnos -m <model file> -d <data file> [-p <path1> <path2> ...] [-v]" << std::endl;
  std::cout << std::endl;
  std::cout << "\t-m, --model <model file>:         filename of the BPMN model" << std::endl;
  std::cout << "\t-d, --data <data file>:           filename of the CSV containing the instance data" << std::endl;
  std::cout << "\t-p, --paths <path1> <path2> ...:  paths in which lookup tables can be found" << std::endl;
  std::cout << "\t-v, --verbose:                    display the execution log" << std::endl;
  exit(1);
}

struct Arguments {
  Arguments() : verbose(false) {};
  std::string modelFile;
  std::string dataFile;
  std::vector<std::string> paths;
  bool verbose;
};

Arguments parse_arguments(int argc, char* argv[]) {
  Arguments args;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if ((arg == "--model" || arg == "-m") && i + 1 < argc) {
      args.modelFile = argv[++i];
    }
    else if ((arg == "--data" || arg == "-d") && i + 1 < argc) {
      args.dataFile = argv[++i];
    }
    else if ((arg == "--paths" || arg == "-p") && i + 1 < argc) {
      while (i + 1 < argc && argv[i + 1][0] != '-') {
        args.paths.push_back(argv[++i]);
      }
    }
    else if ((arg == "--verbose" || arg == "-v")) {
      args.verbose = true;
    }
    else {
      std::cerr << "Unknown parameter: " << arg << "\n";
      print_usage();
    }
  }

  if (args.modelFile.empty() || args.dataFile.empty()) {
    std::cerr << "Error: --model and --data are required.\n";
    print_usage();
  }

  return args;
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    print_usage();
  }

  Arguments args = parse_arguments(argc, argv);
  
  BPMNOS::Model::LookupTable::folders = args.paths;
  BPMNOS::Model::StaticDataProvider dataProvider(args.modelFile,args.dataFile);
  auto scenario = dataProvider.createScenario();

  BPMNOS::Execution::Engine engine;
  BPMNOS::Execution::ReadyHandler readyHandler;
  BPMNOS::Execution::DeterministicTaskCompletion completionHandler;
  readyHandler.connect(&engine);
  completionHandler.connect(&engine);

  BPMNOS::Execution::GuidedEvaluator evaluator;
  BPMNOS::Execution::GreedyController controller(&evaluator);
  controller.connect(&engine);
      
  BPMNOS::Execution::MyopicMessageTaskTerminator messageTaskTerminator;
  BPMNOS::Execution::TimeWarp timeHandler;
  messageTaskTerminator.connect(&engine);
  timeHandler.connect(&engine);

  auto createRecorder = [&args]() -> BPMNOS::Execution::Recorder {
    if (args.verbose) {
      return BPMNOS::Execution::Recorder(std::cout);
    }
    else {
      return BPMNOS::Execution::Recorder();
    }
  };
  auto recorder = createRecorder();
  recorder.subscribe(&engine);

  engine.run(scenario.get());

  return 0;
}

