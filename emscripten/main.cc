#include "main/lsp/wrapper.h"
#include "main/realmain.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
// so that we can compile this file with normal C++ compiler
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace std;

namespace {

unique_ptr<sorbet::realmain::lsp::SingleThreadedLSPWrapper> wrapper;

void typecheckString(const char *rubySrc) {
    const char *argv[] = {"sorbet", "--color=always", "--silence-dev-message", "-e", rubySrc};
    sorbet::realmain::realmain(size(argv), const_cast<char **>(reinterpret_cast<const char **>(argv)));
}

} // namespace

extern "C" {
void EMSCRIPTEN_KEEPALIVE typecheck(const char *optionsJson) {
    rapidjson::Document options;
    options.Parse(optionsJson);
    if (options.HasParseError()) {
        fmt::print(stderr, "emscripten/main.cc: Failed to parse JSON from JavaScript: '{}'\n", optionsJson);
        fmt::print(stderr, "emscripten/main.cc: Falling back to assuming input was Ruby source.\n", optionsJson);
        return typecheckString(optionsJson);
    }

    if (!options.IsArray()) {
        fmt::print(stderr, "JSON from JavaScript was not an array: '{}'\n", optionsJson);
        fmt::print(stderr, "emscripten/main.cc: Falling back to assuming input was Ruby source.\n", optionsJson);
        return typecheckString(optionsJson);
    }

    vector<string> argStrings;
    for (rapidjson::SizeType i = 0; i < options.Size(); i++) {
        const auto &argI = options[i];
        if (!argI.IsString()) {
            fmt::print(stderr, "JSON from JavaScript was not a String at element {} of array: '{}'\n", i, optionsJson);
            fmt::print(stderr, "emscripten/main.cc: Falling back to assuming input was Ruby source.\n", optionsJson);
            return typecheckString(optionsJson);
        }

        argStrings.push_back(argI.GetString());
    }

    vector<char *> argCharStars;
    argCharStars.reserve(argStrings.size());
    for (size_t i = 0; i < argStrings.size(); ++i) {
        argCharStars.push_back(const_cast<char *>(argStrings[i].c_str()));
    }

    sorbet::realmain::realmain(argCharStars.size(), argCharStars.data());
}

void EMSCRIPTEN_KEEPALIVE initializeLsp(const char *optionsJson) {
    rapidjson::Document options;

    options.Parse(optionsJson);

    if (options.HasParseError()) {
        fmt::print(stderr, "emscripten/main.cc: Failed to parse JSON from JavaScript: '{}'\n", optionsJson);
        fmt::print(stderr, "emscripten/main.cc: Falling back to assuming input was Ruby source.\n", optionsJson);
    }

    auto opts = make_shared<sorbet::realmain::options::Options>();
    if (options.HasMember("--enable-experimental-rbs-comments")) {
        opts->cacheSensitiveOptions.rbsEnabled = true;
    } else if (options.HasMember("--enable-experimental-rbs-comments=true")) {
        opts->cacheSensitiveOptions.rbsEnabled = true;
    } else if (options.HasMember("--enable-experimental-rbs-comments=false")) {
        opts->cacheSensitiveOptions.rbsEnabled = false;
    }

    if (!wrapper) {
        wrapper = sorbet::realmain::lsp::SingleThreadedLSPWrapper::create(std::string_view(), opts);
    }

    wrapper->enableAllExperimentalFeatures();
}

void EMSCRIPTEN_KEEPALIVE lsp(void (*respond)(const char *), const char *message) {
    auto responses = wrapper->getLSPResponsesFor(message);
    for (auto &response : responses) {
        respond(response->toJSON().c_str());
    }
}

int main(int argc, char **argv) {}
}
