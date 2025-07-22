#include <JuceHeader.h>
#include "Util/FileUtil.h"
#include "Util/ErrorMessage.h"
#include "Util/CommonDefs.h"
#include "Util/StringUtil.h"
#include "Util/GuiUtil.h"
#include "Build/PlatformRpi4.h"
#include "Build/LaunchProcess.h"

#include "Resources/bsp/bsp_RPI4B.h"

#if defined(LINUX)
#include "Resources/BinaryToolsRpi4Linux.h"
#elif defined(MACOS)
#include "Resources/BinaryToolsRpi4Macos.h"
#elif defined(WINDOWS)
#include "Resources/BinaryToolsRpi4Win64A.h"
#include "Resources/BinaryToolsRpi4Win64B.h"
#include "Resources/BinaryToolsRpi4Win64C.h"
#include "Resources/BinaryToolsRpi4Win64D.h"
#else
#error Unsupported OS in Platform.cpp for RASPPI
#endif

#define IP_ADDRESS "192.168.1.27"

using namespace stride;
using namespace juce;

namespace platform {

static int g_binarySizeBytes = -1;
static std::string g_programmingFilePath;
static float g_programmingProgress = 0.0f;

PlatformRpi4b::PlatformRpi4b(PlatformEnum platformEnum)
: PlatformBase(platformEnum)
{
    configPlatform();
}

PlatformRpi4b::~PlatformRpi4b()
{

}

void PlatformRpi4b::configPlatform()
{
    m_platformConfig.TOOLCHAIN_PREFIX     ="aarch64-none-elf";
    m_platformConfig.TOOLCHAIN_VERSION    = "12.2.1";
    m_platformConfig.BUILD_OUTPUT_BINARY  = "Avalon.img";
    m_platformConfig.PROGRAMMING_FILE     = "Avalon.img";
    m_platformConfig.LINKER_FILENAME      = "linker.ld";
    m_platformConfig.AVALON_AUX_FUNCTIONS = "";

    m_platformConfig.BASE_CPU_LOAD_PERCENT  = 2.0f;
    m_platformConfig.BASE_RAM0_LOAD_PERCENT = 0.1f;
    m_platformConfig.BASE_RAM1_LOAD_PERCENT = 0.1f;
    m_platformConfig.BASE_AUDIO_BUFFERS     = 6;

    m_platformConfig.PROGRAM_FLASH_MAX_SIZE    = 33554432; // 15 MiB
    m_platformConfig.PROGRAM_RAM_SIZE          = 536870912; // 512 MiB
    m_platformConfig.PROGRAM_RAM0_SAFETY_RATIO = 0.90f;    // linker calculated RAM cannot exceed 90%
    m_platformConfig.PROGRAM_RAM1_SAFETY_RATIO = 0.98f;    // linker calculated RAM cannot exceed 98%
    m_platformConfig.CPU_SAFETY_THRESHOLD      = 95.0f;    // CPU estimatd limit cannot exceed 95%
    m_platformConfig.COMMON_SAFETY_RATIO       = 0.90f;    // general purpose safety threshold

    m_platformConfig.productName = "STRIDE-MKII";
    m_platformConfig.mcuTypeName = "RPI4B";
}

std::vector<std::string> PlatformRpi4b::getExtraIncludeLibs()
{
    std::vector<std::string> libs;
    libs.push_back("arm_math");
    return libs;
}

const char* PlatformRpi4b::getCoreIncludesZip() { return bsp_RPI4B::includes_RPI4B_zip; }
size_t      PlatformRpi4b::getCoreIncludesZipSize() { return bsp_RPI4B::includes_RPI4B_zipSize; }
const char* PlatformRpi4b::getCoreLibsZip() { return bsp_RPI4B::libs_RPI4B_zip; }
size_t      PlatformRpi4b::getCoreLibsZipSize() {return bsp_RPI4B::libs_RPI4B_zipSize; }

std::string PlatformRpi4b::getLinkerFile()
{
    constexpr char BUILD_LINKER_FILE[] = "\
ENTRY(_start)\n\
\n\
SECTIONS\n\
{\n\
	.init : {\n\
		*(.init)\n\
	}\n\
\n\
	.text : {\n\
		*(.text*)\n\
\n\
		_etext = .;\n\
	}\n\
\n\
	.rodata : {\n\
		*(.rodata*)\n\
	}\n\
\n\
	.init_array : {\n\
		__init_start = .;\n\
\n\
		KEEP(*(.init_array*))\n\
\n\
		__init_end = .;\n\
	}\n\
\n\
	.ARM.exidx : {\n\
		__exidx_start = .;\n\
\n\
		*(.ARM.exidx*)\n\
\n\
		__exidx_end = .;\n\
	}\n\
\n\
	.eh_frame : {\n\
		*(.eh_frame*)\n\
	}\n\
\n\
	.data : {\n\
		*(.data*)\n\
	}\n\
\n\
	.bss : {\n\
		__bss_start = .;\n\
\n\
		*(.bss*)\n\
		*(COMMON)\n\
\n\
		_end = .;\n\
		end = .;\n\
	}\n\
}\n\
";
    return std::string(BUILD_LINKER_FILE);
}

std::string PlatformRpi4b::getMakefile()
{
#if defined(LINUX)
constexpr char BUILD_MAKEFILE[] = "\
CPPFILT	= $(TOOL_PREFIX)c++filt\n\
ARCHCPU	?= -DAARCH=64 -mcpu=cortex-a72 -mlittle-endian\n\
CPPFLAGS += -ffreestanding -fno-rtti\n\
CPPFLAGS += $(ARCHCPU)\n\
CPPFLAGS += -DREALTIME -DDEFAULT_KEYMAP=\"US\" -D__circle__=450100 -DRASPPI=4 -DRASPPI4 -DSTDLIB_SUPPORT=1 -D__VCCOREVER__=0x04000000\n\
CPPFLAGS += -U__unix__ -U__linux__\n\
CPPFLAGS += -DSYSPLATFORM_STD_MUTEX\n\
CPPFLAGS += -D__GNUC_PYTHON__\n\
#CPPFLAGS += -DARDUINO=10815 -DTEENSYDUINO -D__arm__\n\
\n\
LIBGCC    = \"$(shell $(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libgcc.a)\"\n\
LIBC      = \"$(shell $(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libc.a)\"\n\
LIBM	  = \"$(shell $(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libm.a)\"\n\
LIBNOSYS  = \"$(shell $(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libnosys.a)\"\n\
LIBSTDCPP = \"$(shell $(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libstdc++.a)\"\n\
CIRCLE_LIBS += $(LIBSTDCPP) $(LIBM) $(LIBC) $(LIBGCC) $(LIBNOSYS)\n\
\n\
INCLUDE_DIRS_LIST +=\n\
INCLUDE_DIRS += $(addprefix -I./include/, $(INCLUDE_DIRS_LIST)) \n\
CPPFLAGS += $(INCLUDE_DIRS)\n\
CFLAGS +=\n\
CXXFLAGS += -Wno-aligned-new\n\
\n\
LOADADDR = 0x80000\n\
LDFLAGS += -O2 --gc-sections --relax --section-start=.init=$(LOADADDR)\n\
\n\
SYS_STAT_LIBS += --whole-archive $(addprefix -l:, $(DATAPAK_LIST)) --no-whole-archive\n\
\n\
all: $(TARGET)\n\
%.o: %.cpp\n\
\t$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RELEASEFLAGS) -c -o $@ $<\n\
$(TARGET): $(OBJ_FILES)\n\
\t$(LD) -o $(TARGET).elf -Map $(TARGET).map $(LDFLAGS) $(LD_FILE) \\\n\
\t\t$(CRTBEGIN) $(OBJ_FILES) $(SYS_STAT_LIBS) $(CORE_LIBS) \\\n\
\t--start-group $(CIRCLE_LIBS) --end-group $(CRTEND)\n\
\t$(OBJDUMP) -d $(TARGET).elf | $(CPPFILT) > $(TARGET).lst\n\
\t$(OBJCOPY) $(TARGET).elf -O binary $(TARGET).img\n\
\t$-cp $(TARGET).img kernel84.img\n\
clean:\n\
\t-rm -f $(OBJ_FILES)\n\
\t-rm -f $(TARGET)\n\
\n";
return std::string(BUILD_MAKEFILE);
#elif defined(WINDOWS)
#error "Windows is not yet supported for RPI4 platform"
#elif defined(MACOS)
#error "MacOS is not yet supported for RPI4 platform"
#else
#error "No supported OS defined"
#endif


}

std::string PlatformRpi4b::createTestMakefile(const std::string& toolsDirectory, const std::string& libsDirectory,
    const std::string& datFilename, const std::string& testAppName, const std::string& irDataName,
    const std::vector<std::string>& includeDirectoriesVec
    )
{
    static const std::string NEWLINE = "\n";
    std::string coreFilenameStr = std::string("core." +
                std::to_string(CORELIB_MAJOR_VER) + "." + std::to_string(CORELIB_MINOR_VER) + "." + std::to_string(CORELIB_PATCH_VER) +
                ".dat");

    std::string makefileStr;

#ifdef AVALON_REV2
    makefileStr += "export AVALON_REV=2\n";
#else
    #error "Supported AVALON_REV is not defined"
#endif

#if defined(LINUX) || defined(MACOS)
    makefileStr += "COMPILER_PATH = " + toolsDirectory + "/bin/\n";
    makefileStr += "TOOL_PREFIX=" + m_platformConfig.TOOLCHAIN_PREFIX + "-" + NEWLINE;

    makefileStr += "\
BASE_DIR = $(CURDIR)\n\
PATH +=:$(COMPILER_PATH)\n\
ARCH=aarch64\n\
";
    makefileStr += "INCLUDE_DIRS = ";
    for (auto& directory : includeDirectoriesVec) {
        makefileStr += "-I" + directory + " ";
    }
    makefileStr += NEWLINE;
    makefileStr += "LIBS_DIR = " + libsDirectory + NEWLINE;
    makefileStr += "EFX_FILE = " + datFilename + NEWLINE;
    makefileStr += "CORE_FILENAME = " + coreFilenameStr + NEWLINE;

    makefileStr += "\
CC      = $(TOOL_PREFIX)gcc\n\
CXX     = $(TOOL_PREFIX)g++\n\
LD      = $(TOOL_PREFIX)ld\n\
OBJCOPY = $(TOOL_PREFIX)objcopy\n\
OBJDUMP = $(TOOL_PREFIX)objdump\n\
CPPFILT	= $(TOOL_PREFIX)c++filt\n\
ARCHCPU	?= -DAARCH=64 -mcpu=cortex-a72 -mlittle-endian\n\
CPPFLAGS += -c -Wall -fsigned-char -ffreestanding $(COMMON_FLAGS)\n\
CPPFLAGS += -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti\n\
CPPFLAGS += -Wno-error=narrowing\n\
CPPFLAGS += $(ARCHCPU)\n\
CPPFLAGS += -DREALTIME -DDEFAULT_KEYMAP=\"US\" -D__circle__=450100 -DRASPPI=4 -DSTDLIB_SUPPORT=1 -D__VCCOREVER__=0x04000000\n\
CPPFLAGS += -U__unix__ -U__linux__\n\
CPPFLAGS += -DSYSPLATFORM_STD_MUTEX\n\
CPPFLAGS += -DPROCESS_SERIAL_MIDI\n\
CPPFLAGS += -DAUDIO_BLOCK_SAMPLES=128 -DAUDIO_SAMPLE_RATE_EXACT=48000.0f\n\
CPPFLAGS += -D__GNUC_PYTHON__\n\
LIBGCC    = \"$(shell $(COMPILER_PATH)/$(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libgcc.a)\"\n\
LIBC      = \"$(shell $(COMPILER_PATH)/$(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libc.a)\"\n\
LIBM	  = \"$(shell $(COMPILER_PATH)/$(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libm.a)\"\n\
LIBNOSYS  = \"$(shell $(COMPILER_PATH)/$(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libnosys.a)\"\n\
LIBSTDCPP = \"$(shell $(COMPILER_PATH)/$(TOOL_PREFIX)gcc $(ARCHCPU) -print-file-name=libstdc++.a)\"\n\
CIRCLE_LIBS += $(LIBSTDCPP) $(LIBM) $(LIBC) $(LIBGCC) $(LIBNOSYS)\n\
DEFAULTFLAGS = -O2 -D NDEBUG -DUSB_MIDI_AUDIO_SERIAL\n\
\n\
CPPFLAGS += -DRASPPI4 -DARDUINO=10815 -DTEENSYDUINO -D__arm__\n\
ifeq ($(AVALON_REV),2)\n\
CPPFLAGS += -DAVALON_REV2\n\
endif\n\
CPPFLAGS += $(INCLUDE_DIRS)\n\
CXXFLAGS += -std=gnu++17 -fpermissive -fno-rtti -fno-threadsafe-statics -felide-constructors\n\
LOADADDR = 0x80000\n\
LDFLAGS += -O2 --gc-sections --relax --section-start=.init=$(LOADADDR)\n\
";
    makefileStr += "LD_FILE  = -T./" + m_platformConfig.LINKER_FILENAME + NEWLINE;
    makefileStr += "\
LDFLAGS  += -L./lib -L../efx\n\
LDFLAGS  += -L./ -L$(LIBS_DIR)\n\
";
    makefileStr += "\
CORE_LIBS = -l:$(CORE_FILENAME)\n\
";
    makefileStr += std::string("TARGET_HEXNAME=") + testAppName + std::string(".hex\n");
    makefileStr += "all: " + testAppName + NEWLINE + NEWLINE;
    makefileStr += std::string("%.o:") + std::string("%.cpp") + NEWLINE;
    makefileStr += "\
\t$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEFAULTFLAGS) $(INCLUDE_DIRS) -c -o $@ $<\n\
\n\
";
    makefileStr += testAppName + ": " + testAppName + ".o " + irDataName + ".o\n";
    makefileStr += "\t$(LD) $(COMMON_FLAGS) -o " + testAppName + " $(LDFLAGS) $(LD_FILE) " +
        testAppName + ".o " + irDataName + ".o -l:$(EFX_FILE) $(CORE_LIBS) --start-group $(CIRCLE_LIBS) --end-group\n";
    makefileStr += "clean:" + NEWLINE;
    makefileStr += "\t-rm -rf " + testAppName + " " + testAppName + ".o " + irDataName + ".o \n";

#elif defined(WINDOWS)
#error "Windows is not supported yet for RPI4"
#else
#error Unsupported OS in Makefile.cpp
#endif

    return makefileStr;
}   

int PlatformRpi4b::unzipBuildTools(const std::string& toolsDirectory) {

    if (toolsDirectory.empty()) { errorMessage("::unzipTools(): toolsDirectory is empty"); return FAILURE; }

    if (FileUtil::directoryExists(toolsDirectory)) {
        // TODO maybe do a recursive file count to validate the tools are extracted properly
        return SUCCESS;
    } // tools already extracted

    int result = FileUtil::createDirectory(toolsDirectory);
    if (result != SUCCESS) { errorMessage("::unzipTools(): unable to create tool directory"); return FAILURE; }

    File toolDir(toolsDirectory);

#if defined(LINUX)
    MemoryInputStream memStream(BinaryToolsRpi4Linux::linuxTools_zip, BinaryToolsRpi4Linux::linuxTools_zipSize, false); // false is to not deep copy the data
    ZipFile zipFile(memStream);

    Result zipResult = zipFile.uncompressTo(toolDir);
    if (zipResult.failed()) {
        FileUtil::deleteDirectory(toolsDirectory); // delete if an error occurs since the dir existence is used to check of the tools are there
        errorMessage("BuildEngine::unzipTools(): fail to extract tool binaries");
        return FAILURE;
    }
#elif defined(MACOS)
    MemoryInputStream memStream(BinaryToolsRpi4Macos::macosTools_zip, BinaryToolsRpi4Macos::macosTools_zipSize, false); // false is to not deep copy the data
    ZipFile zipFile(memStream);

    Result zipResult = zipFile.uncompressTo(toolDir);
    if (zipResult.failed()) {
        FileUtil::deleteDirectory(toolsDirectory); // delete if an error occurs since the dir existence is used to check of the tools are there
        errorMessage("BuildEngine::unzipTools(): fail to extract tool binaries");
        return FAILURE;
    }
#elif defined(WINDOWS)
    {
        MemoryInputStream memStreamA(BinaryToolsRpi4Win64A::win64ToolsA_zip, BinaryToolsRpi4Win64A::win64ToolsA_zipSize, false); // false is to not deep copy the data
        ZipFile zipFile(memStreamA);
        Result zipResult = zipFile.uncompressTo(toolDir);
        if (zipResult.failed()) {
            FileUtil::deleteDirectory(toolsDirectory); // delete if an error occurs since the dir existence is used to check of the tools are there
            errorMessage("BuildEngine::unzipTools(): fail to extract tool binaries A");
            return FAILURE;
        }
    }
    {
        MemoryInputStream memStreamB(BinaryToolsRpi4Win64B::win64ToolsB_zip, BinaryToolsRpi4Win64B::win64ToolsB_zipSize, false); // false is to not deep copy the data
        ZipFile zipFile(memStreamB);
        Result zipResult = zipFile.uncompressTo(toolDir);
        if (zipResult.failed()) {
            FileUtil::deleteDirectory(toolsDirectory); // delete if an error occurs since the dir existence is used to check of the tools are there
            errorMessage("BuildEngine::unzipTools(): fail to extract tool binaries B");
            return FAILURE;
        }
    }
    {
        MemoryInputStream memStreamC(BinaryToolsRpi4Win64C::win64ToolsC_zip, BinaryToolsRpi4Win64C::win64ToolsC_zipSize, false); // false is to not deep copy the data
        ZipFile zipFile(memStreamC);
        Result zipResult = zipFile.uncompressTo(toolDir);
        if (zipResult.failed()) {
            FileUtil::deleteDirectory(toolsDirectory); // delete if an error occurs since the dir existence is used to check of the tools are there
            errorMessage("BuildEngine::unzipTools(): fail to extract tool binaries C");
            return FAILURE;
        }
    }
    {
        MemoryInputStream memStreamD(BinaryToolsRpi4Win64D::win64ToolsD_zip, BinaryToolsRpi4Win64D::win64ToolsD_zipSize, false); // false is to not deep copy the data
        ZipFile zipFile(memStreamD);
        Result zipResult = zipFile.uncompressTo(toolDir);
        if (zipResult.failed()) {
            FileUtil::deleteDirectory(toolsDirectory); // delete if an error occurs since the dir existence is used to check of the tools are there
            errorMessage("BuildEngine::unzipTools(): fail to extract tool binaries D");
            return FAILURE;
        }
    }

#else
#error Unsupported OS in BuildEngine.cpp
#endif

#if defined(LINUX) || defined(MACOS)
    // Repair the executable permissions for the tools
    // It would be nice if JUCE fixed this itself!!!
    auto setExecutePermissions = [](File& binDir) {
        Array<File> listOfBins = binDir.findChildFiles(File::findFiles, false, "*");
        if (listOfBins.size() < 1) {
            errorMessage(std::string("BuildEngine::unzipTools(): unable to find tool binaries for setting executable in "));
        } else {
            for (auto& file : listOfBins) {
                if (file.exists()) {
                    if (!file.setExecutePermission(true)) {
                        errorMessage(std::string("BuildEngine::unzipTools(): error changing execute permission on " + file.getFullPathName().toStdString()));
                    } else {
                        noteMessage(std::string("BuildEngine::unzipTools(): changed execute permission on " + file.getFullPathName().toStdString()));
                    }
                }
            }
        }
    };

    File binDir = File(String(toolsDirectory + "/bin"));
    setExecutePermissions(binDir);

    binDir = File(String(toolsDirectory + "/aarch64-none-elf/bin"));
    setExecutePermissions(binDir);

    binDir =  File(String(toolsDirectory + "/libexec/gcc/aarch64-none-elf/12.2.1"));
    setExecutePermissions(binDir);

#if defined(MACOS)
    File makeFile = File(String(toolsDirectory + "/gmake"));
    makeFile.setExecutePermission(true);
#endif

#endif

    return SUCCESS;
}

size_t PlatformRpi4b::getFlashMaxSize()
{
    return m_platformConfig.PROGRAM_FLASH_MAX_SIZE;
}

// static int getCodeSize(const std::string& toolsDirectory, const std::string& programDir, const std::string& programName,
//     const std::string& sectionName, unsigned lineIndex, unsigned wordIndex, size_t& codeSize)
// {
//     std::string cmd = "cd " + programDir + " && " + toolsDirectory + "/bin/aarch64-none-elf-objdump -h -j " + sectionName + " ./" + programName;
//     std::string result;
//     int ret = LaunchProcessCommand(cmd, result, programDir);
//     if (ret != SUCCESS) { return ret; }

//     std::vector<std::string> tokens = StringUtil::splitByDelimiter(result, "\n");

//     std::vector<std::string> sizeLine = StringUtil::splitByDelimiter(tokens[lineIndex], " ");

//     // strip the leftover spaces
//     while(true) {
//         bool spaceFound = false;
//         for (auto it = sizeLine.begin(); it != sizeLine.end(); ++it) {
//             const char* ch = it->c_str();
//             if (*ch == 0) {
//                 sizeLine.erase(it);
//                 spaceFound = true;
//                 break;
//             }
//         }
//         if (!spaceFound) { break; }
//     }
//     std::string codeSizeStr = sizeLine[wordIndex];
//     codeSize = std::stoul(codeSizeStr, nullptr, 16);

//     return SUCCESS;
// }

bool PlatformRpi4b::isProgramRamValid(const std::string& toolsDirectory, const std::string& programDir, const std::string& programName,
    float& ram0Min, float& ram1Min)
{
    return true;
    // size_t itcm=0, data=0, bss=0, bssDma=0;
    // getCodeSize(toolsDirectory, programDir, programName, ".text.itcm", 5, 2, itcm);  // RAM0 program code (FASTRUN)
    // getCodeSize(toolsDirectory, programDir, programName, ".data", 5, 2, data);  // RAM0 initialized variables
    // getCodeSize(toolsDirectory, programDir, programName, ".bss", 5, 2, bss);  // RAM0 uninitialized variables
    // getCodeSize(toolsDirectory, programDir, programName, ".bss.dma", 5, 2, bssDma); // RAM1 DMA variables (DMAMEM)

    // size_t ram0BytesUsed = itcm + data + bss;
    // size_t ram1BytesUsed = bssDma;
    // float ram0Usage = (float)ram0BytesUsed / (float)PROGRAM_RAM_SIZE;
    // float ram1Usage = (float)ram1BytesUsed / (float)PROGRAM_RAM_SIZE;

    // char textBuf[256];
    // snprintf(textBuf, 255, "platform::isProgramRamValid(): itcm:%08X  data:%08X  bss:%08X  bss.dma:%08X", (unsigned)itcm, (unsigned)data, (unsigned)bss, (unsigned)bssDma);
    // noteMessage(std::string(textBuf));
    // snprintf(textBuf, 255, "platform:isProgramRamValid(): Estimated RAM0 usage is %08X / %08X, %f%%", (unsigned)ram0BytesUsed, (unsigned)PROGRAM_RAM_SIZE, (float)(ram0BytesUsed) / (float)(PROGRAM_RAM_SIZE) * 100.0f);
    // noteMessage(std::string(textBuf));
    // snprintf(textBuf, 255, "platform:isProgramRamValid(): Estimated RAM1 usage is %08X / %08X, %f%%", (unsigned)ram1BytesUsed, (unsigned)PROGRAM_RAM_SIZE, (float)ram1BytesUsed / (float)PROGRAM_RAM_SIZE * 100.0f);
    // noteMessage(std::string(textBuf));

    // ram0Min = ram0Usage;
    // ram1Min = ram1Usage;

    // if ((ram0Usage >= PROGRAM_RAM0_SAFETY_RATIO) || (ram1Usage >= PROGRAM_RAM1_SAFETY_RATIO)) { return false; }
    // else { return true; }
}

bool PlatformRpi4b::isProgramFlashValid(const std::string& toolsDirectory, const std::string& programDir, const std::string& programName)
{
    return true;
    // size_t progMemSizeBytes = 0;
    // getCodeSize(toolsDirectory, programDir, programName, ".text.progmem", 5, 2, progMemSizeBytes);

    // float progMemUsage = (float)progMemSizeBytes / (float)PROGRAM_FLASH_MAX_SIZE;

    // char textBuf[256];
    // snprintf(textBuf, 255, "platform::isProgramFlashValid(): progmem:%08X, usage:%f%%", (unsigned)progMemSizeBytes, progMemUsage * 100.0f);
    // noteMessage(std::string(textBuf));

    // if (progMemUsage >= COMMON_SAFETY_RATIO) { return false; }
    // else { return true; }
}

std::string PlatformRpi4b::getEfxMakefileInc(const Flags flags, const std::string& cppFlags)
{
    std::string commonFlags = "COMMON_FLAGS +=";
    if (flags.noPrintf) { commonFlags += " -DNO_EFX_PRINTF"; }

    std::string defaultFlags;
    if (flags.isDebug) { defaultFlags = "DEFAULTFLAGS   = $(DEBUGFLAGS)"; }
    else                       { defaultFlags = "DEFAULTFLAGS   = $(RELEASEFLAGS)"; }

    std::string makefileIncStr;

#if defined(LINUX) || defined(MACOS)
    #if defined(NDEBUG)
    makefileIncStr += "TMOD=@\n";
    #endif
    makefileIncStr += "COMPILER_PATH = $(CURDIR)/tools/bin/" + NEWLINE;
    makefileIncStr += "TOOL_PREFIX=" + m_platformConfig.TOOLCHAIN_PREFIX + "-" + NEWLINE;

    makefileIncStr += "\
BASE_DIR = $(CURDIR)\n\
PATH +=:$(COMPILER_PATH)\n\
ARCH=aarch64\n\
";
    makefileIncStr += "PLATFORM_NAME=" + m_platformConfig.productName + NEWLINE;
    makefileIncStr += "\
INCLUDE_PATH = $(CURDIR)/extinc\n\
SRCDIR = $(BASE_DIR)/src\n\
OBJDIR = $(BASE_DIR)/obj\n\
INCDIR = $(BASE_DIR)/inc\n\
EFXDIR=$(BASE_DIR)/../../efx\n\
OUTPUT_DIRS=$(EFXDIR) $(OBJDIR)\n\
MKDIR_P = mkdir -p\n\
" + NEWLINE;

    makefileIncStr += "CC      = $(TOOL_PREFIX)gcc\n\
CXX     = $(TOOL_PREFIX)g++\n\
AS      = $(TOLL_PREFIX)as\n\
AR      = $(TOOL_PREFIX)gcc-ar\n\
LD      = $(TOOL_PREFIX)ld\n\
OBJCOPY = $(TOOL_PREFIX)objcopy\n\
OBJDUMP = $(TOOL_PREFIX)objdump\n\
CPPFILT	= $(TOOL_PREFIX)c++filt\n\
ARCHCPU	?= -DAARCH=64 -mcpu=cortex-a72 -mlittle-endian\n\
";

    makefileIncStr += "\n# Compiler and Linker settings\n";
    makefileIncStr += commonFlags + NEWLINE;
    makefileIncStr += "\
\n\
# Preprocessor flags\n\
CPPFLAGS += -c -Wall -fsigned-char -ffreestanding $(COMMON_FLAGS)\n\
CPPFLAGS += -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti\n\
CPPFLAGS += -Wno-error=narrowing\n\
CPPFLAGS += $(ARCHCPU)\n\
CPPFLAGS += -DREALTIME -DDEFAULT_KEYMAP=\"US\" -D__circle__=450100 -DRASPPI=4 -DSTDLIB_SUPPORT=1 -D__VCCOREVER__=0x04000000\n\
CPPFLAGS += -U__unix__ -U__linux__\n\
CPPFLAGS += -DSYSPLATFORM_STD_MUTEX\n\
CPPFLAGS += -DPROCESS_SERIAL_MIDI\n\
CPPFLAGS += -DAUDIO_BLOCK_SAMPLES=128 -DAUDIO_SAMPLE_RATE_EXACT=48000.0f\n\
CPPFLAGS += -D__GNUC_PYTHON__\n\
";
    makefileIncStr += std::string("CPPFLAGS += ") + cppFlags + NEWLINE;
    makefileIncStr += "\
CPPFLAGS += -DRASPPI4 -DARDUINO=10815 -DTEENSYDUINO -D__arm__\n\
INCLUDE_PATHS = -I$(INCLUDE_PATH) -I$(INCLUDE_PATH)/cores -I$(BASE_DIR)/inc/$(TARGET_NAME) -I$(BASE_DIR)/src -I$(BASE_DIR)/src/inc\n\
CPPFLAGS += $(INCLUDE_PATHS)\n\
\n\
ifeq ($(AVALON_REV),2)\n\
CPPFLAGS += -DAVALON_REV2\n\
endif\n\
\n\
RPI4LIBS_INCLUDE_LIST = arm_math globalCompat sysPlatformRpi4 Avalon Stride Audio\n\
RPI4LIBS_COMMA_LIST = \"arm_math,globalCompat,sysPlatfromRpi4,Avalon,Stride,Audio\"\n\
\n\
RPI4LIBS_INCLUDE_PATHS = $(addprefix -I$(INCLUDE_PATH)/, $(RPI4LIBS_INCLUDE_LIST))\n\
INCLUDE_PATHS += $(RPI4LIBS_INCLUDE_PATHS)\n\
CPPFLAGS += $(RPI4LIBS_INCLUDE_PATHS)\n\
\n\
CFLAGS   += -std=gnu99 $(COMMON_FLAGS)\n\
CXXFLAGS += -std=gnu++17 -fpermissive -fno-rtti -fno-threadsafe-statics -felide-constructors\n\
\n\
# Archiver flags\n\
ARFLAGS   = -cr\n\
\n\
DEBUGFLAGS     = -g -O0 -D_DEBUG -DUSB_DUAL_SERIAL\n\
RELEASEFLAGS   = -s -fvisibility=hidden -O3 -D NDEBUG -DUSB_MIDI_AUDIO_SERIAL";
if (flags.enableFastMath) { makefileIncStr += " -ffast-math";}
if (flags.enableO3) { makefileIncStr += " -O3\n"; }
else                { makefileIncStr += " -O2\n"; }
    makefileIncStr += defaultFlags + NEWLINE;
    makefileIncStr += "\nSTATIC_TARGET_LIST = $(TARGET_NAME).$(PLATFORM_NAME).dat\n\
\n\
API_HEADERS = $(addprefix $(INCDIR)/, $(API_HEADER_LIST))\n\
\n\
SOURCES_CPP = $(addprefix $(SRCDIR)/, $(CPP_SRC_LIST))\n\
SOURCES_C = $(addprefix $(SRCDIR)/, $(C_SRC_LIST))\n\
SOURCES_S = $(addprefix $(SRCDIR)/, $(S_SRC_LIST))\n\
\n\
OBJECTS_CPP = $(addsuffix .o, $(addprefix $(OBJDIR)/, $(CPP_SRC_LIST)))\n\
OBJECTS_C = $(addsuffix .o, $(addprefix $(OBJDIR)/, $(C_SRC_LIST)))\n\
OBJECTS_S = $(addsuffix .o, $(addprefix $(OBJDIR)/, $(S_SRC_LIST)))\n\
\n\
PREPROC_DEFINES = $(addprefix -D, $(PREPROC_DEFINES_LIST))\n\
CPPFLAGS += $(PREPROC_DEFINES)\n\
" + NEWLINE;

    makefileIncStr += "STATIC_TARGET = $(EFXDIR)/$(STATIC_TARGET_LIST)\n" + NEWLINE;

    makefileIncStr += "all: directories api_headers $(STATIC_TARGET)\n\
\n\
directories:\n\
\t$(TMOD)$(MKDIR_P) $(OUTPUT_DIRS)\n\
\n\
api_headers:\n\
\t$(TMOD)-cp -f $(API_HEADERS) $(EFXDIR)\n\
\n\
$(STATIC_TARGET): $(OBJECTS_C) $(OBJECTS_CPP) $(OBJECTS_S)\n\
\t$(AR) $(ARFLAGS) $(STATIC_TARGET) $(OBJECTS_C) $(OBJECTS_CPP) $(OBJECTS_S)\n\
\n\
$(OBJDIR)%.cpp.o: $(SRCDIR)%.cpp\n\
\t$(TMOD)$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEFAULTFLAGS) -c -o $@ $<\n\
\n\
$(OBJDIR)%.c.o: $(SRCDIR)%.c\n\
\t$(TMOD)$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFAULTFLAGS) -c -o $@ $<\n\
\n\
$(OBJDIR)%.S.o: $(SRCDIR)%.S\n\
\t$(TMOD)$(CC) $(CPPFLAGS) -x assembler-with-cpp $(DEFAULTFLAGS) -c -o $@ $<\n\
\n\
clean:\n\
\t$(TMOD)-rm -f $(OBJECTS_C) $(OBJECTS_CPP) $(OBJECTS_S)\n\
\t$(TMOD)-rm -f $(DYN_TARGET) $(STATIC_TARGET)\n\
\t$(TMOD)-rm -f $(EFXDIR)/*.h $(EFXDIR)/*.efx\n\
\t$(TMOD)-rm -f $(ZIPDIR)/$(TARGET_NAME).zip\n\
printvar:\n\
\t$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))\n\
.PHONY: directories api_headers clean printvar\n\
" + NEWLINE;

    return makefileIncStr;

// WINDOWS
#elif defined(WINDOWS)
    return makefileIncStr;
#else
#error Unsupported OS in Makefile.cpp
#endif
}

int  PlatformRpi4b::loadBinaryFile(const std::string& binaryFilePath)
{
    size_t binarySize = FileUtil::getFileSize(binaryFilePath);
    g_binarySizeBytes = binarySize;
    g_programmingFilePath = binaryFilePath;
    return binarySize;
}

int  PlatformRpi4b::openUsb() {
    // send HDLC reset command
    return SUCCESS;
}

int PlatformRpi4b::programDevice() {

    // TODO: we need to replace tftp with uploading over the USB serial
    // connection to the ESP32

    g_programmingProgress = 0.0f;

    std::string ipAddress = std::string(IP_ADDRESS);
    std::string buildFolder = FileUtil::getFolderFromPath(g_programmingFilePath);
    std::string command = "tftp -m binary " + ipAddress + " -c put kernel84.img";

    std::string result;
    std::string msg;
    LaunchProcessCommand(command, result, buildFolder, 60 /* TIMEOUT SECONDS*/);
    if (result.find("Error") != std::string::npos) {
        std::string msg = std::string(__DATE__) + ": BuildThread::run(): BUILD ERROR\n";
        msg += result;
        errorMessage(msg);
        return FAILURE;
    } else {
        msg = "*** " + std::string(__DATE__) + ": RESULT: *** \n" + result;
        noteMessage(msg);
        g_programmingProgress = 1.0f;
        return SUCCESS;
    }

#if 0 // this doesn't work yet
    GetUserDataPrompt1 userPrompt;
    userPrompt.windowTitle = "Device Address";
    userPrompt.windowText = "Please enter the IP address for your STRIDE MKII";
    userPrompt.setPrompt("Please enter the device IP address");
    userPrompt.promptDefaultText = "192.168.1.?";
    userPrompt.triggerPrompt();
    while (true) {
        if (userPrompt.alertWindowPtr) {
            //if (!userPrompt.alertWindowPtr->isCurrentlyModal()) { break; }
            if (userPrompt.isExited()) { break; }
        }
    }
    std::cout << "Programming IP address is " << userPrompt.getResponse() << std::endl;
    return SUCCESS;
#endif
}

float PlatformRpi4b::getProgrammingProgress()
{
    return g_programmingProgress;
}

void PlatformRpi4b::requestProgramThreadExit()
{

}

bool PlatformRpi4b::isEraseDone()
{
    return true;
}

}
