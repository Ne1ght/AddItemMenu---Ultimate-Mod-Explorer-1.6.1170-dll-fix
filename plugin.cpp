#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace {
    void ReportPlugins() {
        auto* console = RE::ConsoleLog::GetSingleton();
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!console || !dataHandler) {
            return;
        }

        // Full plugins: .esm / .esp
        for (auto* file : dataHandler->files) {
            SKSE::log::info("[full]  {}", file->fileName);
            console->Print("[full] %s", file->fileName);
        }

        // Light plugins: .esl and ESL-flagged .esp
        const auto lightCount = dataHandler->GetLoadedLightModCount();
        for (auto* file : dataHandler->files) {
            if (file->IsLight()) {
                continue;  // already listed in the light section
            }
            SKSE::log::info("[full]  {}", file->fileName);
            console->Print("[full] %s", file->fileName);
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            ReportPlugins();
        }
    });

    return true;
}