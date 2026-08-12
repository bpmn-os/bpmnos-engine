#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <bpmn++.h>
#include <bpmnos-model.h>
#include <bpmnos-execution.h>

void print_usage() {
  std::cout << "Usage:" << std::endl;
  std::cout << "\tbpmnos-greedy --model <model file> --data <data file> [--json <json file>] [--provider {static|expected|dynamic|stochastic}] [--seed <number>] [--evaluator {local|guided}] [--folders <folder1> <folder2> ...] [--bisection] [--starttime <number>] [--endtime <number>] [--verbose]" << std::endl;
  std::cout << "\tbpmnos-greedy -m <model file> -d <data file> [-j <json file>] [-p <provider>]  [-s <number>] [-e <evaluator>] [-f <folder1> <folder2> ...] [--b] [-tmin <number>] [-tmax <number>] [-v]" << std::endl;
  std::cout << std::endl;
  std::cout << "\t-m, --model <model file>:             name of the BPMN model file" << std::endl;
  std::cout << "\t-d, --data <data file>:               name of the CSV file containing the instance data" << std::endl;
  std::cout << "\t-j, --json <json file>:               name of the file for the JSON output" << std::endl;
  std::cout << "\t-p, --provider {static|expected|dynamic|stochastic} (default: stochastic)" << std::endl;
  std::cout << "\t-s, --seed <number>:                  seed for stochastic scenarios (default: random)" << std::endl;
  std::cout << "\t-e, --evaluator {local|guided}:       evaluator for decisions (default: guided)" << std::endl;
  std::cout << "\t-f, --folders <folder1> <folder2> ...: folders in which lookup tables can be found" << std::endl;
  std::cout << "\t-b, --bisection:                      use bisection for choices" << std::endl;
  std::cout << "\t-tmin, --starttime <number>:          simulation time at which execution begins (default: earliest instantiation)" << std::endl;
  std::cout << "\t-tmax, --endtime <number>:            simulation time when execution is terminated" << std::endl;
  std::cout << "\t-v, --verbose:                        display the execution log" << std::endl;
  exit(1);
}

struct Arguments {
  Arguments() : verbose(false) {};
  std::string modelFile;
  std::string dataFile;
  std::string jsonFile;
  std::string providerName = "stochastic";
  unsigned int seed = std::random_device{}();
  std::string evaluatorName = "guided";
  std::vector<std::string> folders;
  bool bisection = false;
  std::optional<BPMNOS::number> tmin;
  std::optional<BPMNOS::number> tmax;
  bool verbose = false;
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
    else if ((arg == "--json" || arg == "-j") && i + 1 < argc) {
      args.jsonFile = argv[++i];
    }
    else if ((arg == "--provider" || arg == "-p") && i + 1 < argc) {
      args.providerName = argv[++i];
    }
    else if ((arg == "--seed" || arg == "-s") && i + 1 < argc) {
      args.seed = (unsigned int)std::stoul(argv[++i]);
    }
    else if ((arg == "--evaluator" || arg == "-e") && i + 1 < argc) {
      args.evaluatorName = argv[++i];
    }
    else if ((arg == "--folders" || arg == "-f") && i + 1 < argc) {
      while (i + 1 < argc && argv[i + 1][0] != '-') {
        args.folders.push_back(argv[++i]);
      }
    }
    else if ((arg == "--bisection" || arg == "-b")) {
      args.bisection = true;
    }
    else if ((arg == "--starttime" || arg == "-tmin") && i + 1 < argc) {
      args.tmin = BPMNOS::to_number(std::string(argv[++i]),BPMNOS::STRING);
    }
    else if ((arg == "--endtime" || arg == "-tmax") && i + 1 < argc) {
      args.tmax = BPMNOS::to_number(std::string(argv[++i]),BPMNOS::STRING);
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

  auto createDataProvider = [&args]() -> std::unique_ptr<BPMNOS::Model::DataProvider> {
    if (args.providerName == "static") {
      return std::make_unique<BPMNOS::Model::StaticDataProvider>(args.modelFile,args.folders,args.dataFile);
    }
    else if (args.providerName == "expected") {
      return std::make_unique<BPMNOS::Model::ExpectedValueDataProvider>(args.modelFile,args.folders,args.dataFile);
    }
    else if (args.providerName == "dynamic") {
      return std::make_unique<BPMNOS::Model::DynamicDataProvider>(args.modelFile,args.folders,args.dataFile);
    }
    else if (args.providerName == "stochastic") {
      return std::make_unique<BPMNOS::Model::StochasticDataProvider>(args.modelFile,args.folders,args.dataFile, args.seed);
    }
    else {
      std::cerr << "Error: unknown data provider.\n";
      print_usage();
    }
    return nullptr;
  };

  auto createEvaluator = [&args]() -> std::shared_ptr<BPMNOS::Execution::Evaluator> {
    if (args.evaluatorName == "local") {
      return std::make_shared<BPMNOS::Execution::LocalEvaluator>();
    }
    else if (args.evaluatorName == "guided") {
      return std::make_shared<BPMNOS::Execution::GuidedEvaluator>();
    }
    else {
      std::cerr << "Error: unknown evaluator.\n";
      print_usage();
    }
    return nullptr;
  };
  
  auto dataProvider = createDataProvider();
  if (args.providerName == "stochastic") {
    std::cout << "Seed: " << args.seed  << std::endl;
  }
  auto scenario = dataProvider->createScenario();

  BPMNOS::Execution::Engine engine;

  auto evaluator = createEvaluator();
  BPMNOS::Execution::GreedyController controller(evaluator, { .bisection = args.bisection });
  controller.connect(&engine);
      
  BPMNOS::Execution::TimeWarp timeHandler;
  timeHandler.connect(&engine);


  std::ofstream jsonStream;
  std::unique_ptr<BPMNOS::Execution::Recorder> recorder;
  if (!args.jsonFile.empty()) {
    jsonStream.open(args.jsonFile);
    if (!jsonStream.is_open()) {
      std::cerr << "Error: unable to open JSON output file: " << args.jsonFile << "\n";
      return 1;
    }
    recorder = std::make_unique<BPMNOS::Execution::Recorder>(BPMNOS::Execution::Recorder::Config{ .stream = jsonStream, .tagged = true });
    recorder->subscribe(&engine);
  }

  std::unique_ptr<BPMNOS::Execution::Recorder> logger;
  if (args.verbose) {
    logger = std::make_unique<BPMNOS::Execution::Recorder>(BPMNOS::Execution::Recorder::Config{ .stream = std::cout, .colored = true });
    logger->subscribe(&engine);
  }


  BPMNOS::Execution::OutcomeSentinel sentinel;
  sentinel.subscribe(&engine);

  // a run beginning before its first instance would only tick through empty instants, so the default start
  // is where the scenario's data starts
  auto earliestInstantiationTime = scenario->getEarliestInstantiationTime();
  if (args.tmin.has_value() && args.tmin.value() > earliestInstantiationTime) {
    // an instance is instantiated at the instant its instantiation time is reached, so a later start
    // would leave every earlier instance uncreated
    std::cerr << "Error: start time " << (double)args.tmin.value() << " is later than the earliest instantiation time " << (double)earliestInstantiationTime << "\n";
    return 1;
  }
  auto startTime = args.tmin.value_or( earliestInstantiationTime );

  if (args.tmax.has_value()) {
    engine.run(scenario.get(), startTime, args.tmax.value());
  }
  else {
    engine.run(scenario.get(), startTime);
  }
  
  logger.reset();
  std::cout << "Status: " << BPMNOS::Execution::outcome[(size_t)sentinel.getOutcome()] << std::endl;
  
  auto objective = (float)engine.getSystemState()->getObjective();
  std::cout << "Objective (maximization): " << objective << std::endl;
  std::cout << "Objective (minimization): " << -objective  << std::endl;
  return 0;
}

