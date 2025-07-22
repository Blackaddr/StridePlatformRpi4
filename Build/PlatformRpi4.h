#pragma once

#include <string>
#include <vector>

#include "Resources/CoreVersion.h"
#include "Build/Platform.h"

namespace platform {

class PlatformRpi4b : public PlatformBase {
public:

    PlatformRpi4b() = delete;
    PlatformRpi4b(PlatformEnum platformEnum);
    virtual ~PlatformRpi4b();

    void configPlatform() override;

    const char* getCoreIncludesZip() override;
    size_t      getCoreIncludesZipSize() override;
    const char* getCoreLibsZip() override;
    size_t      getCoreLibsZipSize() override;

    int unzipBuildTools(const std::string& toolsDirectory) override;

    std::string getLinkerFile() override;
	std::string getMakefile() override;

    std::string createTestMakefile(const std::string& toolsDirectory, const std::string& libsDirectory,
        const std::string& datFilename, const std::string& testAppName, const std::string& irDataName,
        const std::vector<std::string>& includeDirectoriesVec
        ) override;

    std::string getEfxMakefileInc(const Flags compilerFlags, const std::string& cppFlags) override;
    std::vector<std::string> getExtraIncludeLibs() override;

    size_t getFlashMaxSize() override;

    bool isProgramRamValid(const std::string& toolsDirectory, const std::string& programDir, const std::string& programName,
        float& ram0Min, float& ram1Min) override;
    bool isProgramFlashValid(const std::string& toolsDirectory, const std::string& programDir, const std::string& programName) override;

    int  loadBinaryFile(const std::string& binaryFilePath) override;
    int  openUsb() override;
    int  programDevice() override;
    void requestProgramThreadExit() override;
    float getProgrammingProgress() override;
    bool isEraseDone() override;
};

}
