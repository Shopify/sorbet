#include "core/ErrorQueue.h"
#include "core/Unfreeze.h"
#include "main/pipeline/pipeline.h"
#include "payload/payload.h"
#include "spdlog/sinks/stdout_sinks.h"
#include <cstddef>
#include <cstdint>
#include <fstream>

using namespace std;
using namespace sorbet;

auto logger = spdlog::stdout_logger_mt("console");
auto typeErrorsConsole = spdlog::stdout_logger_mt("typeErrorsConsole");

realmain::options::Options createOptions(realmain::options::Parser parser) {
    realmain::options::Options opts;
    opts.parser = parser;
    return opts;
}

realmain::options::Options originalOpts = createOptions(realmain::options::Parser::ORIGINAL);
realmain::options::Options prismOpts = createOptions(realmain::options::Parser::PRISM);

unique_ptr<const OwnedKeyValueStore> kvstore;

unique_ptr<core::GlobalState> buildInitialGlobalState(const realmain::options::Options &opts) {
    auto gs = make_unique<core::GlobalState>(make_shared<core::ErrorQueue>(*typeErrorsConsole, *logger));
    payload::createInitialGlobalState(*gs, opts, kvstore);
    realmain::pipeline::setGlobalStateOptions(*gs, opts);
    return gs;
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
    logger->set_level(spdlog::level::critical);
    typeErrorsConsole->set_level(spdlog::level::critical);
    fatalLogger = logger;

    return 0;
}

enum class ParserResult {
    SUCCESS,
    EXCEPTION,
};

struct PipelineOutput {
    ParserResult result;
    string desugarTreeOutput;
};

PipelineOutput runPipeline(core::GlobalState &gs, const realmain::options::Options &opts, const string &inputData,
                           WorkerPool &workers) {
    try {
        vector<core::FileRef> inputFiles;
        core::FileRef file;
        {
            core::UnfreezeFileTable fileTableAccess(gs);
            file = gs.enterFile("fuzz.rb", inputData);
            inputFiles.emplace_back(file);
            file.data(gs).strictLevel = core::StrictLevel::True;
        }

        auto indexed = realmain::pipeline::index(gs, absl::MakeSpan(inputFiles), opts, workers, kvstore);
        ENFORCE(indexed.hasResult(), "Cancellation is not supported with fuzzing");

        string desugarTreeOutput;
        if (!indexed.result().empty()) {
            desugarTreeOutput = indexed.result()[0].tree.toStringWithTabs(gs, 0);
        }

        // We don't run this fuzzer with any packager options, so we can skip pipeline::package()
        auto foundHashes = nullptr;
        indexed =
            move(realmain::pipeline::nameAndResolve(gs, move(indexed.result()), opts, workers, foundHashes).result());
        realmain::pipeline::typecheck(gs, move(indexed.result()), opts, workers);
        return PipelineOutput{ParserResult::SUCCESS, move(desugarTreeOutput)};
    } catch (SorbetException &) {
        return PipelineOutput{ParserResult::EXCEPTION, ""};
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    static unique_ptr<core::GlobalState> commonGsOriginal;
    static unique_ptr<core::GlobalState> commonGsPrism;
    static unique_ptr<WorkerPool> workers;
    static size_t testCount = 0;

    if (testCount == 0) {
        commonGsOriginal = buildInitialGlobalState(originalOpts);
        commonGsPrism = buildInitialGlobalState(prismOpts);
        workers = WorkerPool::create(0, *logger);
    }

    testCount++;
    string inputData((const char *)data, size);

    // Test with ORIGINAL parser
    PipelineOutput originalOutput;
    {
        auto gsOriginal = commonGsOriginal->deepCopyGlobalState(true);
        originalOutput = runPipeline(*gsOriginal, originalOpts, inputData, *workers);
    }

    // Test with PRISM parser
    PipelineOutput prismOutput;
    {
        auto gsPrism = commonGsPrism->deepCopyGlobalState(true);
        prismOutput = runPipeline(*gsPrism, prismOpts, inputData, *workers);
    }

    // Report differential behavior
    if (originalOutput.result != prismOutput.result) {
        const char *failedParser = (prismOutput.result == ParserResult::EXCEPTION) ? "PRISM" : "ORIGINAL";
        const char *succeededParser = (prismOutput.result == ParserResult::EXCEPTION) ? "ORIGINAL" : "PRISM";

        ENFORCE(false, "{} parser crashed but {} parser succeeded", failedParser, succeededParser);
    }

    if (originalOutput.result == ParserResult::SUCCESS && prismOutput.result == ParserResult::SUCCESS) {
        if (originalOutput.desugarTreeOutput != prismOutput.desugarTreeOutput) {
            string diffDir = "fuzz_parser_diffs/diff-" + to_string(testCount);

            system(("mkdir -p " + diffDir).c_str());

            string inputFilename = diffDir + "/input.rb";
            ofstream inputFile(inputFilename);
            if (inputFile.is_open()) {
                inputFile.write(inputData.c_str(), inputData.size());
                inputFile.close();
            }

            string originalFilename = diffDir + "/original.txt";
            ofstream originalFile(originalFilename);
            if (originalFile.is_open()) {
                originalFile << originalOutput.desugarTreeOutput;
                originalFile.close();
            }

            string prismFilename = diffDir + "/prism.txt";
            ofstream prismFile(prismFilename);
            if (prismFile.is_open()) {
                prismFile << prismOutput.desugarTreeOutput;
                prismFile.close();
            }
        }
    }

    return 0;
}
