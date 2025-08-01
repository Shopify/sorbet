#include "doctest/doctest.h"
#include <cxxopts.hpp>
// has to go first as it violates our requirements

// has to go first, as it violates poisons
#include "core/proto/proto.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "ast/ast.h"
#include "ast/desugar/Desugar.h"
#include "ast/desugar/PrismDesugar.h"
#include "ast/treemap/treemap.h"
#include "cfg/CFG.h"
#include "cfg/builder/builder.h"
#include "class_flatten/class_flatten.h"
#include "common/FileOps.h"
#include "common/common.h"
#include "common/sort/sort.h"
#include "common/strings/formatting.h"
#include "common/web_tracer_framework/tracing.h"
#include "core/Error.h"
#include "core/ErrorCollector.h"
#include "core/ErrorQueue.h"
#include "core/Unfreeze.h"
#include "core/errors/namer.h"
#include "core/errors/resolver.h"
#include "core/serialize/serialize.h"
#include "definition_validator/validator.h"
#include "infer/infer.h"
#include "local_vars/local_vars.h"
#include "main/autogen/autogen.h"
#include "main/autogen/crc_builder.h"
#include "main/autogen/data/definitions.h"
#include "main/autogen/data/version.h"
#include "main/minimize/minimize.h"
#include "main/pipeline/pipeline.h"
#include "namer/namer.h"
#include "packager/packager.h"
#include "parser/parser.h"
#include "parser/prism/Parser.h"
#include "payload/binary/binary.h"
#include "payload/payload.h"
#include "rbs/AssertionsRewriter.h"
#include "rbs/SigsRewriter.h"
#include "resolver/resolver.h"
#include "rewriter/rewriter.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "test/helpers/expectations.h"
#include "test/helpers/position_assertions.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace std;
namespace sorbet::test {

string singleTest;

constexpr string_view whitelistedTypedNoneTest = "missing_typed_sigil.rb"sv;
constexpr string_view packageFileName = "__package.rb"sv;

class CFGCollectorAndTyper {
public:
    vector<unique_ptr<cfg::CFG>> cfgs;
    void preTransformMethodDef(core::Context ctx, ast::ExpressionPtr &tree) {
        auto &m = ast::cast_tree_nonnull<ast::MethodDef>(tree);
        if (!infer::Inference::willRun(ctx, m.declLoc, m.symbol)) {
            return;
        }

        auto cfg = cfg::CFGBuilder::buildFor(ctx.withOwner(m.symbol), m);
        auto symbol = cfg->symbol;
        cfg = infer::Inference::run(ctx.withOwner(symbol), move(cfg));
        if (cfg) {
            for (auto &extension : ctx.state.semanticExtensions) {
                extension->typecheck(ctx, ctx.file, *cfg, m);
            }
        }
        cfgs.push_back(move(cfg));
    }
};

UnorderedSet<string> knownExpectations = {"parse-tree",
                                          "parse-tree-json",
                                          "parse-tree-json-with-locs",
                                          "parse-tree-whitequark",
                                          "rbs-rewrite-tree",
                                          "desugar-tree",
                                          "desugar-tree-raw",
                                          "rewrite-tree",
                                          "rewrite-tree-raw",
                                          "index-tree",
                                          "index-tree-raw",
                                          "symbol-table",
                                          "symbol-table-raw",
                                          "name-tree",
                                          "name-tree-raw",
                                          "resolve-tree",
                                          "resolve-tree-raw",
                                          "flatten-tree",
                                          "flatten-tree-raw",
                                          "cfg",
                                          "cfg-raw",
                                          "cfg-text",
                                          "autogen",
                                          "document-symbols",
                                          "package-tree",
                                          "document-formatting-rubyfmt",
                                          "autocorrects",
                                          "minimized-rbi",
                                          "rbi-gen"};

ast::ParsedFile testSerialize(core::GlobalState &gs, ast::ParsedFile expr) {
    auto &savedFile = expr.file.data(gs);
    auto saved = core::serialize::Serializer::storeTree(savedFile, expr);
    auto restored = core::serialize::Serializer::loadTree(gs, savedFile, saved.data());
    return {move(restored), expr.file};
}

/** Converts a Sorbet Error object into an equivalent LSP Diagnostic object. */
unique_ptr<Diagnostic> errorToDiagnostic(const core::GlobalState &gs, const core::Error &error) {
    if (!error.loc.exists()) {
        return nullptr;
    }
    return make_unique<Diagnostic>(Range::fromLoc(gs, error.loc), error.header);
}

class ExpectationHandler {
    Expectations &test;
    shared_ptr<core::ErrorQueue> &errorQueue;
    shared_ptr<core::ErrorCollector> &errorCollector;

public:
    vector<unique_ptr<core::Error>> errors;
    UnorderedMap<string_view, string> got;

    ExpectationHandler(Expectations &test, shared_ptr<core::ErrorQueue> &errorQueue,
                       shared_ptr<core::ErrorCollector> &errorCollector)
        : test(test), errorQueue(errorQueue), errorCollector(errorCollector){};

    void addObserved(const core::GlobalState &gs, string_view expectationType, std::function<string()> mkExp,
                     bool addNewline = true) {
        if (test.expectations.contains(expectationType)) {
            got[expectationType].append(mkExp());
            if (addNewline) {
                got[expectationType].append("\n");
            }
            drainErrors(gs);
        }
    }

    void checkExpectations(string prefix = "") const {
        for (const auto &gotPhase : got) {
            auto expectation = test.expectations.find(gotPhase.first);
            REQUIRE_MESSAGE(expectation != test.expectations.end(),
                            prefix << "missing expectation for " << gotPhase.first);
            REQUIRE_MESSAGE(expectation->second.size() == 1,
                            prefix << "found unexpected multiple expectations of type " << gotPhase.first);

            auto checker = test.folder + expectation->second.begin()->second;
            auto expect = FileOps::read(checker);

            CHECK_EQ_DIFF(expect, gotPhase.second,
                          fmt::format("{}Mismatch on: {}\n"
                                      "\n"
                                      "If these changes are expected, run this script and commit the results:\n"
                                      "tools/scripts/update_testdata_exp.sh\n",
                                      prefix, checker));
            if (expect == gotPhase.second) {
                MESSAGE(gotPhase.first << " OK");
            }
        }
    }

    void drainErrors(const core::GlobalState &gs) {
        // Moves errors from being owned by GlobalState to having been flushed by the flusher
        // In our case, errorCollector is our error flusher (accumulates a vector, instead of
        // printing to stdout).
        errorQueue->flushAllErrors(gs);
        // Retrieves the collected errors, and sets it to empty again
        auto newErrors = errorCollector->drainErrors();
        // Stores them in ourself, for use with ErrorAssertion::checkAll at a later point.
        errors.insert(errors.end(), make_move_iterator(newErrors.begin()), make_move_iterator(newErrors.end()));
    }

    void dropErrors(const core::GlobalState &gs) {
        // Moves errors from being owned by GlobalState to having been flushed by the flusher
        // In our case, errorCollector is our error flusher (accumulates a vector, instead of
        // printing to stdout).
        errorQueue->flushAllErrors(gs);
        // Retrieves the collected errors, sets it to empty again, and then drops those errors.
        auto _newErrors = errorCollector->drainErrors();
    }

    void clear(const core::GlobalState &gs) {
        got.clear();
        errorQueue->flushAllErrors(gs);
        auto _newErrors = errorCollector->drainErrors();
    }
};

vector<ast::ParsedFile> index(core::GlobalState &gs, absl::Span<core::FileRef> files, ExpectationHandler &handler,
                              Expectations &test) {
    vector<ast::ParsedFile> trees;
    for (auto file : files) {
        auto fileName = FileOps::getFileName(file.data(gs).path());
        if (fileName != whitelistedTypedNoneTest && (fileName != packageFileName || !file.data(gs).source().empty()) &&
            file.data(gs).source().find("# typed:") == string::npos) {
            ADD_FAIL_CHECK_AT(file.data(gs).path().data(), 1, "Add a `# typed: strict` line to the top of this file");
        }

        // if a file is typed: ignore, then we shouldn't try to parse
        // it or do anything with it at all
        if (file.data(gs).strictLevel == core::StrictLevel::Ignore) {
            ast::ParsedFile pf{ast::make_expression<ast::EmptyTree>(), file};
            trees.emplace_back(move(pf));
            continue;
        }

        parser::Parser::ParseResult parseResult;
        switch (parser) {
            case realmain::options::Parser::ORIGINAL: {
                core::UnfreezeNameTable nameTableAccess(gs); // enters original strings
                auto settings = parser::Parser::Settings{false, false, false, gs.cacheSensitiveOptions.rbsEnabled};
                parseResult = parser::Parser::run(gs, file, settings);
                break;
            }
            case realmain::options::Parser::PRISM: {
                core::UnfreezeNameTable nameTableAccess(gs); // enters original strings

                auto nodes = parser::Prism::Parser::run(gs, file);
                parseResult = parser::Parser::ParseResult{move(nodes), {}};
                break;
            }
        }

        auto nodes = move(parseResult.tree);

        handler.drainErrors(gs);
        handler.addObserved(gs, "parse-tree", [&]() { return nodes->toString(gs); });
        handler.addObserved(gs, "parse-tree-whitequark", [&]() { return nodes->toWhitequark(gs); });
        handler.addObserved(gs, "parse-tree-json", [&]() { return nodes->toJSON(gs); });

        {
            if (gs.cacheSensitiveOptions.rbsEnabled) {
                core::UnfreezeNameTable nameTableAccess(gs); // enters original strings
                core::MutableContext ctx(gs, core::Symbols::root(), file);
                auto associator = rbs::CommentsAssociator(ctx, parseResult.commentLocations);
                auto commentMap = associator.run(nodes);

                auto rbsSignatures = rbs::SigsRewriter(ctx, commentMap.signaturesForNode);
                nodes = rbsSignatures.run(std::move(nodes));

                auto rbsAssertions = rbs::AssertionsRewriter(ctx, commentMap.assertionsForNode);
                nodes = rbsAssertions.run(std::move(nodes));
            }

            handler.addObserved(gs, "rbs-rewrite-tree", [&]() { return nodes->toString(gs); });
        }

        // Desugarer
        ast::ParsedFile desugared;
        switch (parser) {
            case realmain::options::Parser::ORIGINAL: {
                core::UnfreezeNameTable nameTableAccess(gs); // enters original strings

                core::MutableContext ctx(gs, core::Symbols::root(), file);
                desugared = testSerialize(gs, ast::ParsedFile{ast::desugar::node2Tree(ctx, move(nodes)), file});
                break;
            }
            case realmain::options::Parser::PRISM: {
                core::UnfreezeNameTable nameTableAccess(gs); // enters original strings

                core::MutableContext ctx(gs, core::Symbols::root(), file);
                desugared = testSerialize(gs, ast::ParsedFile{ast::prismDesugar::node2Tree(ctx, move(nodes)), file});
                break;
            }
        }

        handler.addObserved(gs, "desugar-tree", [&]() { return desugared.tree.toString(gs); });
        handler.addObserved(gs, "desugar-tree-raw", [&]() { return desugared.tree.showRaw(gs); });

        ast::ParsedFile localNamed;

        // Rewriter
        ast::ParsedFile rewritten;
        {
            core::UnfreezeNameTable nameTableAccess(gs); // enters original strings

            core::MutableContext ctx(gs, core::Symbols::root(), desugared.file);
            bool previous = gs.cacheSensitiveOptions.runningUnderAutogen;
            gs.cacheSensitiveOptions.runningUnderAutogen = test.expectations.contains("autogen");
            rewritten =
                testSerialize(gs, ast::ParsedFile{rewriter::Rewriter::run(ctx, move(desugared.tree)), desugared.file});
            gs.cacheSensitiveOptions.runningUnderAutogen = previous;
        }

        handler.addObserved(gs, "rewrite-tree", [&]() { return rewritten.tree.toString(gs); });
        handler.addObserved(gs, "rewrite-tree-raw", [&]() { return rewritten.tree.showRaw(gs); });

        {
            core::UnfreezeNameTable nameTableAccess(gs); // possibly enters mangled names
            core::MutableContext ctx(gs, core::Symbols::root(), desugared.file);
            localNamed = testSerialize(gs, local_vars::LocalVars::run(ctx, move(rewritten)));
        }

        handler.addObserved(gs, "index-tree", [&]() { return localNamed.tree.toString(gs); });
        handler.addObserved(gs, "index-tree-raw", [&]() { return localNamed.tree.showRaw(gs); });

        trees.emplace_back(move(localNamed));
    }

    return trees;
}

void package(core::GlobalState &gs, unique_ptr<WorkerPool> &workers, absl::Span<ast::ParsedFile> trees,
             ExpectationHandler &handler, vector<shared_ptr<RangeAssertion>> &assertions) {
    if (!gs.packageDB().enabled()) {
        return;
    }

    // Packager runs over all trees.
    packager::Packager::run(gs, *workers, trees);
    for (auto &tree : trees) {
        handler.addObserved(gs, "package-tree", [&]() {
            return fmt::format("# -- {} --\n{}", tree.file.data(gs).path(), tree.tree.toString(gs));
        });
    }
}

void name(core::GlobalState &gs, absl::Span<ast::ParsedFile> trees, WorkerPool &workers) {
    core::UnfreezeNameTable nameTableAccess(gs);     // creates singletons and class names
    core::UnfreezeSymbolTable symbolTableAccess(gs); // enters symbols
    auto foundHashes = nullptr;
    auto canceled = namer::Namer::run(gs, trees, workers, foundHashes);
    ENFORCE(!canceled);
}

TEST_CASE("PerPhaseTest") { // NOLINT
    Expectations test = Expectations::getExpectations(singleTest);

    auto inputPath = test.folder + test.basename;
    auto rbName = test.basename + ".rb";

    for (auto &exp : test.expectations) {
        if (!knownExpectations.contains(exp.first)) {
            FAIL_CHECK("Unknown pass: " << exp.first);
        }
    }

    auto assertions = RangeAssertion::parseAssertions(test.sourceFileContents);
    auto opts = RangeAssertion::parseOptions(assertions);
    opts.censorForSnapshotTests = true;
    opts.stripePackagesHint = "PACKAGE_ERROR_HINT";
    opts.parser = sorbet::test::parser;

    auto logger = spdlog::stderr_color_mt("fixtures: " + inputPath);
    auto workers = WorkerPool::create(0, *logger);
    auto errorCollector = make_shared<core::ErrorCollector>();
    auto errorQueue = make_shared<core::ErrorQueue>(*logger, *logger, errorCollector);
    auto gs = make_unique<core::GlobalState>(errorQueue);

    unique_ptr<const OwnedKeyValueStore> kvstore = nullptr;
    payload::createInitialGlobalState(*gs, opts, kvstore);
    realmain::pipeline::setGlobalStateOptions(*gs, opts);

    for (auto provider : sorbet::pipeline::semantic_extension::SemanticExtensionProvider::getProviders()) {
        gs->semanticExtensions.emplace_back(provider->defaultInstance());
    }

    unique_ptr<core::GlobalState> emptyGs;
    if (!test.minimizeRBI.empty() || test.expectations.contains("rbi-gen")) {
        // Copy GlobalState after initializing it, but before rest of pipeline, so that it
        // represents an "empty" GlobalState.
        emptyGs = gs->deepCopyGlobalState();
    }

    // Read files
    vector<core::FileRef> files;
    {
        core::UnfreezeFileTable fileTableAccess(*gs);

        for (auto &sourceFile : test.sourceFiles) {
            auto fref = gs->enterFile(test.sourceFileContents[test.folder + sourceFile]);
            if (FileOps::getFileName(sourceFile) == whitelistedTypedNoneTest) {
                fref.data(*gs).strictLevel = core::StrictLevel::False;
            }
            if (FileOps::getFileName(sourceFile) == packageFileName && fref.data(*gs).source().empty()) {
                fref.data(*gs).strictLevel = core::StrictLevel::False;
            }
            files.emplace_back(fref);
        }
    }

    ExpectationHandler handler(test, errorQueue, errorCollector);

    vector<ast::ParsedFile> trees;
    auto filesSpan = absl::Span<core::FileRef>(files);
    if (opts.cacheSensitiveOptions.stripePackages) {
        auto numPackageFiles = realmain::pipeline::partitionPackageFiles(*gs, filesSpan);
        auto inputPackageFiles = filesSpan.first(numPackageFiles);
        filesSpan = filesSpan.subspan(numPackageFiles);

        trees = index(*gs, inputPackageFiles, handler, test);
    }

    auto nonPackageTrees = index(*gs, filesSpan, handler, test);
    name(*gs, absl::Span<ast::ParsedFile>(trees), *workers);
    package(*gs, workers, absl::Span<ast::ParsedFile>(trees), handler, assertions);
    name(*gs, absl::Span<ast::ParsedFile>(nonPackageTrees), *workers);
    package(*gs, workers, absl::Span<ast::ParsedFile>(nonPackageTrees), handler, assertions);
    realmain::pipeline::unpartitionPackageFiles(trees, move(nonPackageTrees));

    for (auto &tree : trees) {
        // Namer
        auto namedTree = testSerialize(*gs, move(tree));

        handler.addObserved(*gs, "name-tree", [&]() { return namedTree.tree.toString(*gs); });
        handler.addObserved(*gs, "name-tree-raw", [&]() { return namedTree.tree.showRaw(*gs); });

        tree = move(namedTree);
    }

    if (test.expectations.contains("autogen")) {
        {
            core::UnfreezeNameTable nameTableAccess(*gs);
            core::UnfreezeSymbolTable symbolAccess(*gs);

            trees = resolver::Resolver::runConstantResolution(*gs, move(trees), *workers);
        }
        handler.addObserved(
            *gs, "autogen",
            [&]() {
                stringstream payload;
                auto crcBuilder = autogen::CRCBuilder::create();
                for (auto &tree : trees) {
                    core::Context ctx(*gs, core::Symbols::root(), tree.file);
                    auto pf = autogen::Autogen::generate(ctx, move(tree), autogen::AutogenConfig{{}}, *crcBuilder);
                    payload << pf.toString(ctx, autogen::AutogenVersion::MAX_VERSION);
                }
                return payload.str();
            },
            false);

        handler.checkExpectations();

        // Autogen forces you to to put --stop-after=namer so lets not run
        // anything else
        return;
    } else {
        core::UnfreezeNameTable nameTableAccess(*gs);     // Resolver::defineAttr
        core::UnfreezeSymbolTable symbolTableAccess(*gs); // enters stubs
        trees = move(resolver::Resolver::run(*gs, move(trees), *workers).result());

        if (opts.cacheSensitiveOptions.stripePackages) {
            trees = packager::VisibilityChecker::run(*gs, *workers, move(trees));
        }

        handler.drainErrors(*gs);
    }

    handler.addObserved(*gs, "symbol-table", [&]() { return gs->toString(); });
    handler.addObserved(*gs, "symbol-table-raw", [&]() { return gs->showRaw(); });

    for (auto &resolvedTree : trees) {
        handler.addObserved(*gs, "resolve-tree", [&]() { return resolvedTree.tree.toString(*gs); });
        handler.addObserved(*gs, "resolve-tree-raw", [&]() { return resolvedTree.tree.showRaw(*gs); });
    }

    if (!test.minimizeRBI.empty()) {
        auto gsForMinimize = emptyGs->deepCopyGlobalState();
        auto opts = realmain::options::Options{};
        opts.parser = parser;
        auto minimizeRBI = test.folder + test.minimizeRBI;
        realmain::Minimize::indexAndResolveForMinimize(*gs, *gsForMinimize, opts, *workers, minimizeRBI);
        auto printerConfig = realmain::options::PrinterConfig{};
        printerConfig.enabled = true;
        printerConfig.outputPath = "/dev/null"; // tricks PrinterConfig::print into buffering
        printerConfig.supportsFlush = true;
        realmain::Minimize::writeDiff(*gs, *gsForMinimize, printerConfig);

        auto addNewline = false;
        handler.addObserved(
            *gs, "minimized-rbi", [&]() { return printerConfig.flushToString(); }, addNewline);
    }

    // Simulate what pipeline.cc does: We want to start typechecking big files first because it helps with better work
    // distribution
    fast_sort(trees, [&](const auto &lhs, const auto &rhs) -> bool {
        return lhs.file.data(*gs).source().size() > rhs.file.data(*gs).source().size();
    });

    for (auto &resolvedTree : trees) {
        auto file = resolvedTree.file;

        core::Context ctx(*gs, core::Symbols::root(), file);
        resolvedTree = class_flatten::runOne(ctx, move(resolvedTree));

        resolvedTree = definition_validator::runOne(ctx, move(resolvedTree));
        handler.drainErrors(*gs);

        handler.addObserved(*gs, "flatten-tree", [&]() { return resolvedTree.tree.toString(*gs); });
        handler.addObserved(*gs, "flatten-tree-raw", [&]() { return resolvedTree.tree.showRaw(*gs); });

        // Don't run typecheck on RBI files.
        if (resolvedTree.file.data(ctx).isRBI()) {
            continue;
        }

        auto checkTree = [&]() {
            if (resolvedTree.tree == nullptr) {
                auto path = file.data(*gs).path();
                ADD_FAIL_CHECK_AT(path.begin(), 1, "Already used tree. You can only have 1 CFG-ish .exp file");
            }
        };
        auto checkPragma = [&](string ext) {
            if (file.data(*gs).strictLevel < core::StrictLevel::True) {
                auto path = file.data(*gs).path();
                ADD_FAIL_CHECK_AT(path.begin(), 1,
                                  "Missing `# typed:` pragma. Sources with ." << ext
                                                                              << ".exp files must specify # typed:");
            }
        };

        // CFG
        if (test.expectations.contains("cfg") || test.expectations.contains("cfg-raw") ||
            test.expectations.contains("cfg-text")) {
            checkTree();
            checkPragma("cfg");
            CFGCollectorAndTyper collector;
            core::Context ctx(*gs, core::Symbols::root(), resolvedTree.file);
            ast::ShallowWalk::apply(ctx, collector, resolvedTree.tree);
            for (auto &extension : ctx.state.semanticExtensions) {
                extension->finishTypecheckFile(ctx, file);
            }
            resolvedTree.tree.reset();

            handler.addObserved(*gs, "cfg", [&]() {
                stringstream dot;
                dot << "digraph \"" << rbName << "\" {" << '\n';
                for (auto &cfg : collector.cfgs) {
                    dot << cfg->toString(ctx) << '\n' << '\n';
                }
                dot << "}" << '\n';
                return dot.str();
            });

            handler.addObserved(*gs, "cfg-raw", [&]() {
                stringstream dot;
                dot << "digraph \"" << rbName << "\" {" << '\n';
                dot << "  graph [fontname = \"Courier\"];\n";
                dot << "  node [fontname = \"Courier\"];\n";
                dot << "  edge [fontname = \"Courier\"];\n";
                for (auto &cfg : collector.cfgs) {
                    dot << cfg->showRaw(ctx) << '\n' << '\n';
                }
                dot << "}" << '\n';
                return dot.str();
            });

            handler.addObserved(
                *gs, "cfg-text",
                [&]() {
                    stringstream dot;
                    for (auto &cfg : collector.cfgs) {
                        dot << cfg->toTextualString(ctx) << '\n' << '\n';
                    }
                    return dot.str();
                },
                false);
        }

        // If there is a tree left with a typed: pragma, run the inferencer
        if (resolvedTree.tree != nullptr && file.data(*gs).originalSigil >= core::StrictLevel::True) {
            checkTree();
            CFGCollectorAndTyper collector;
            core::Context ctx(*gs, core::Symbols::root(), resolvedTree.file);
            ast::ShallowWalk::apply(ctx, collector, resolvedTree.tree);
            for (auto &extension : ctx.state.semanticExtensions) {
                extension->finishTypecheckFile(ctx, file);
            }
            resolvedTree.tree.reset();
            handler.drainErrors(*gs);
        }
    }

    for (auto &extension : gs->semanticExtensions) {
        extension->finishTypecheck(*gs);
    }

    // Check autocorrects
    {
        auto autocorrects = vector<core::AutocorrectSuggestion>{};
        for (const auto &error : handler.errors) {
            if (error->isSilenced) {
                continue;
            }

            for (const auto &autocorrect : error->autocorrects) {
                autocorrects.push_back(autocorrect);
            }
        }

        auto fs = OSFileSystem{};
        auto applied = core::AutocorrectSuggestion::apply(*gs, fs, autocorrects);

        auto toWrite = UnorderedMap<core::FileRef, string>{};
        for (const auto &[file, editedSource] : applied) {
            toWrite[file] = move(editedSource);
        }

        auto addNewline = false;
        handler.addObserved(
            *gs, "autocorrects",
            [&]() {
                fmt::memory_buffer buf;
                auto sortedFiles = files; // copy
                fast_sort(sortedFiles, [&](const auto &lhs, const auto &rhs) -> bool {
                    return lhs.data(*gs).path() < rhs.data(*gs).path();
                });
                for (const auto &file : sortedFiles) {
                    string editedSource;
                    if (toWrite.contains(file)) {
                        editedSource = toWrite[file];
                    } else {
                        editedSource = file.data(*gs).source();
                    }
                    fmt::format_to(std::back_inserter(buf), "# -- {} --\n{}# ------------------------------\n",
                                   core::File::censorFilePathForSnapshotTests(file.data(*gs).path()), editedSource);
                }
                return to_string(buf);
            },
            addNewline);
    }

    handler.checkExpectations();

    if (test.expectations.contains("symbol-table")) {
        string table = gs->toString() + '\n';
        CHECK_EQ_DIFF(handler.got["symbol-table"], table, "symbol-table should not be mutated by CFG+inference");
    }

    if (test.expectations.contains("symbol-table-raw")) {
        string table = gs->showRaw() + '\n';
        CHECK_EQ_DIFF(handler.got["symbol-table-raw"], table,
                      "symbol-table-raw should not be mutated by CFG+inference");
    }

    // Check warnings and errors
    {
        map<string, vector<unique_ptr<Diagnostic>>> diagnostics;
        for (auto &error : handler.errors) {
            if (error->isSilenced) {
                continue;
            }
            auto diag = errorToDiagnostic(*gs, *error);
            ENFORCE(diag != nullptr, "Error was given no valid location - '{}'", error->toString(*gs));

            auto path = error->loc.file().data(*gs).path();
            diagnostics[string(path.begin(), path.end())].push_back(std::move(diag));
        }
        ErrorAssertion::checkAll(test.sourceFileContents, RangeAssertion::getErrorAssertions(assertions), diagnostics);
    }

    // Allow later phases to have errors that we didn't test for
    errorQueue->flushAllErrors(*gs);
    { auto _ = errorCollector->drainErrors(); }

    // now we test the incremental resolver

    auto disableStressIncremental =
        BooleanPropertyAssertion::getValue("disable-stress-incremental", assertions).value_or(false);
    if (disableStressIncremental) {
        MESSAGE("errors OK");
        return;
    }

    handler.clear(*gs);
    auto symbolsBefore = gs->symbolsUsedTotal();

    vector<ast::ParsedFile> newTrees;
    for (auto &f : trees) {
        if (f.file.data(*gs).strictLevel == core::StrictLevel::Ignore) {
            newTrees.emplace_back(move(f));
            continue;
        }

        const int prohibitedLines = f.file.data(*gs).source().size();
        auto newSource = absl::StrCat(string(prohibitedLines + 1, '\n'), f.file.data(*gs).source());
        auto newFile =
            make_shared<core::File>(string(f.file.data(*gs).path()), move(newSource), f.file.data(*gs).sourceType);
        gs->replaceFile(f.file, move(newFile));

        core::MutableContext ctx(*gs, core::Symbols::root(), f.file);

        // this replicates the logic of pipeline::indexOne
        parser::Parser::ParseResult parseResult;
        switch (parser) {
            case realmain::options::Parser::ORIGINAL: {
                auto settings = parser::Parser::Settings{false, false, false, gs->cacheSensitiveOptions.rbsEnabled};
                parseResult = parser::Parser::run(*gs, f.file, settings);
                break;
            }
            case realmain::options::Parser::PRISM: {
                auto nodes = parser::Prism::Parser::run(ctx, f.file);
                parseResult = parser::Parser::ParseResult{move(nodes), {}};
                break;
            }
        }

        if (gs->cacheSensitiveOptions.rbsEnabled) {
            auto associator = rbs::CommentsAssociator(ctx, parseResult.commentLocations);
            auto commentMap = associator.run(parseResult.tree);

            auto rbsSignatures = rbs::SigsRewriter(ctx, commentMap.signaturesForNode);
            parseResult.tree = rbsSignatures.run(std::move(parseResult.tree));

            auto rbsAssertions = rbs::AssertionsRewriter(ctx, commentMap.assertionsForNode);
            parseResult.tree = rbsAssertions.run(std::move(parseResult.tree));
        }

        auto nodes = move(parseResult.tree);

        handler.addObserved(*gs, "rbs-rewrite-tree", [&]() { return nodes->toString(*gs); });

        ast::ParsedFile file;
        switch (parser) {
            case realmain::options::Parser::ORIGINAL: {
                file = testSerialize(*gs, ast::ParsedFile{ast::desugar::node2Tree(ctx, move(nodes)), f.file});
                break;
            }
            case realmain::options::Parser::PRISM: {
                file = testSerialize(*gs, ast::ParsedFile{ast::prismDesugar::node2Tree(ctx, move(nodes)), f.file});
                break;
            }
        }

        handler.addObserved(*gs, "desguar-tree", [&]() { return file.tree.toString(*gs); });
        handler.addObserved(*gs, "desugar-tree-raw", [&]() { return file.tree.showRaw(*gs); });

        // Rewriter pass
        file = testSerialize(*gs, ast::ParsedFile{rewriter::Rewriter::run(ctx, move(file.tree)), file.file});
        handler.addObserved(*gs, "rewrite-tree", [&]() { return file.tree.toString(*gs); });
        handler.addObserved(*gs, "rewrite-tree-raw", [&]() { return file.tree.showRaw(*gs); });

        // local vars
        file = testSerialize(*gs, local_vars::LocalVars::run(ctx, move(file)));
        handler.addObserved(*gs, "index-tree", [&]() { return file.tree.toString(*gs); });
        handler.addObserved(*gs, "index-tree-raw", [&]() { return file.tree.showRaw(*gs); });

        newTrees.emplace_back(move(file));
    }
    trees = move(newTrees);
    fast_sort(trees, [](auto &lhs, auto &rhs) { return lhs.file < rhs.file; });

    bool ranIncrementalNamer = false;
    {
        // namer
        for (auto &tree : trees) {
            core::UnfreezeSymbolTable symbolTableAccess(*gs);
            vector<ast::ParsedFile> vTmp;
            vTmp.emplace_back(move(tree));
            core::FoundDefHashes foundHashes; // out param, compute this just for test coverage
            // The lsp_test_runner will turn every testdata test into a test of
            // Namer::runIncremental by way of creating a file update with leading whitespace.
            //
            // Here, to complement those tests, we just run Namer::run (not Namer::runIncremental)
            // to stress the codepath where Namer is not tasked with deleting anything when run for
            // the fast path.
            ENFORCE(!ranIncrementalNamer);
            auto canceled = namer::Namer::run(*gs, absl::Span<ast::ParsedFile>(vTmp), *workers, &foundHashes);
            ENFORCE(!canceled);
            tree = testSerialize(*gs, move(vTmp[0]));

            handler.addObserved(*gs, "name-tree", [&]() { return tree.tree.toString(*gs); });
            handler.addObserved(*gs, "name-tree-raw", [&]() { return tree.tree.showRaw(*gs); });
        }
    }

    if (opts.cacheSensitiveOptions.stripePackages) {
        absl::c_stable_partition(trees, [&](const auto &pf) { return pf.file.isPackage(*gs); });
        trees = packager::Packager::runIncremental(*gs, move(trees), *workers);
        for (auto &tree : trees) {
            handler.addObserved(*gs, "package-tree", [&]() {
                return fmt::format("# -- {} --\n{}", tree.file.data(*gs).path(), tree.tree.toString(*gs));
            });
        }
    }

    // resolver
    {
        vector<core::ClassOrModuleRef> recomputeNoSymbols;
        core::UnfreezeNameTable nameTableAccess(*gs);
        core::UnfreezeSymbolTable symbolTableAccess(*gs);
        trees =
            move(resolver::Resolver::runIncremental(*gs, move(trees), ranIncrementalNamer, *workers, recomputeNoSymbols)
                     .result());
    }

    if (opts.cacheSensitiveOptions.stripePackages) {
        trees = packager::VisibilityChecker::run(*gs, *workers, move(trees));
    }

    for (auto &resolvedTree : trees) {
        handler.addObserved(*gs, "resolve-tree", [&]() { return resolvedTree.tree.toString(*gs); });
        handler.addObserved(*gs, "resolve-tree-raw", [&]() { return resolvedTree.tree.showRaw(*gs); });
    }

    handler.checkExpectations("[stress-incremental] ");

    // and drain all the remaining errors
    errorQueue->flushAllErrors(*gs);
    { auto _ = errorCollector->drainErrors(); }

    {
        INFO("the incremental resolver should not add new symbols");
        CHECK_EQ(symbolsBefore, gs->symbolsUsedTotal());
    }
}
} // namespace sorbet::test

int main(int argc, char *argv[]) {
    cxxopts::Options options("test_corpus", "Test corpus for Sorbet typechecker");
    options.allow_unrecognised_options().add_options()("single_test", "run over single test.",
                                                       cxxopts::value<string>()->default_value(""), "testpath");
    options.allow_unrecognised_options().add_options()("parser", "The parser to use while testing.",
                                                       cxxopts::value<string>()->default_value("original"),
                                                       "{original, prism}");
    auto res = options.parse(argc, argv);

    if (res.count("single_test") != 1) {
        printf("--single_test=<filename> argument expected\n");
        return 1;
    }

    sorbet::test::singleTest = res["single_test"].as<string>();

    auto logger = spdlog::stderr_color_mt("test_runner");

    auto parser = sorbet::realmain::options::extractParser(res["parser"].as<string>(), logger);

    if (!parser) {
        return 1;
    }

    sorbet::test::parser = *parser;

    doctest::Context context(argc, argv);
    return context.run();
}
