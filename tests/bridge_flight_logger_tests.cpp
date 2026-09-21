#include "xrfg/bridge_flight_logger.hpp"

#include <windows.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void write(const fs::path& path, const std::string& contents) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << contents;
    require(static_cast<bool>(stream), "fixture write failed");
}

std::string read(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {(std::istreambuf_iterator<char>(stream)), {}};
}

struct TempDirectory {
    fs::path path;

    TempDirectory() {
        std::array<wchar_t, MAX_PATH> root{};
        GetTempPathW(static_cast<DWORD>(root.size()), root.data());
        path = fs::path(root.data()) /
            (L"xrfg-flight-tests-" + std::to_wstring(GetCurrentProcessId()));
        fs::remove_all(path);
        fs::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

} // namespace

int main(int argc, char** argv) {
    if (argc == 2) {
        xrfg::BridgeFlightLogger child;
        child.initialize(argv[1]);
        child.event(xrfg::BridgeFlightOperation::application_wait_frame, 6, 11, 22, 33);
        TerminateProcess(GetCurrentProcess(), 73);
        return 1;
    }
    try {
        TempDirectory temporary;

        xrfg::BridgeFlightLogger disabled;
        disabled.initialize(temporary.path / "disabled");
        require(!disabled.enabled(), "missing INI must keep logging disabled");
        require(disabled.begin(
                    xrfg::BridgeFlightOperation::application_wait_frame)
                    .sequence == 0,
            "disabled logger produced a token");

        const fs::path enabled_directory = temporary.path / "enabled";
        fs::create_directories(enabled_directory);
        write(
            enabled_directory / "ofxr_bridge.ini",
            "[diagnostics]\r\n"
            "logging_enabled=1\r\n"
            "max_file_mb=1\r\n"
            "flush_each_event=0\r\n");

        wchar_t exe[32768]{};
        GetModuleFileNameW(nullptr, exe, 32768);
        std::wstring command = L"\"" + std::wstring(exe) + L"\" \"" + enabled_directory.wstring() + L"\"";
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        require(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0,
            nullptr, nullptr, &startup, &process) != FALSE, "child launch failed");
        const DWORD wait = WaitForSingleObject(process.hProcess, 10000);
        if (wait != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 74);
        DWORD code{};
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        require(wait == WAIT_OBJECT_0 && code == 73, "child did not terminate as expected");
        bool persisted = false;
        for (const auto& item : fs::directory_iterator(enabled_directory)) {
            if (item.path().extension() == ".log") {
                persisted |= read(item.path()).find(
                    "op=app_wait_frame result=6 dur_us=0 a=11 b=22 c=33") != std::string::npos;
            }
        }
        require(persisted, "last recorder event lost after forced termination");

        xrfg::BridgeFlightLogger enabled;
        enabled.initialize(enabled_directory);
        require(enabled.enabled(), "enabled INI did not start bridge logging");
        const fs::path log_path = enabled.log_path();
        require(!log_path.empty() && log_path.parent_path() == enabled_directory,
            "bridge log was not created beside the DLL configuration");

        const auto token = enabled.begin(
            xrfg::BridgeFlightOperation::application_wait_frame,
            11,
            22,
            33);
        require(token.sequence != 0, "enabled logger did not produce a token");
        enabled.end(
            token,
            xrfg::BridgeFlightOperation::application_wait_frame,
            7,
            44,
            55,
            66);
        enabled.event(
            xrfg::BridgeFlightOperation::synthesis_pair,
            9,
            77,
            88,
            99);
        enabled.event(
            xrfg::BridgeFlightOperation::runtime_identity,
            1,
            2,
            3,
            4);
        enabled.event(
            xrfg::BridgeFlightOperation::presenter_transition,
            3);
        enabled.event(
            xrfg::BridgeFlightOperation::presenter_submission,
            0);
        enabled.event(
            xrfg::BridgeFlightOperation::nvidia_gpu_stages,
            2,
            101,
            202,
            303);
        enabled.event(
            xrfg::BridgeFlightOperation::nvidia_gpu_total,
            0,
            404,
            1010,
            77);
        enabled.shutdown();

        const std::string log = read(log_path);
        require(log.find("phase=B op=app_wait_frame") != std::string::npos,
            "bridge log is missing the wait begin boundary");
        require(log.find("phase=E op=app_wait_frame result=7") !=
                std::string::npos,
            "bridge log is missing the matching wait completion");
        require(log.find("phase=I op=synthesis_pair result=9") !=
                std::string::npos,
            "bridge log is missing a synthesis event");
        require(log.find("a=77 b=88 c=99") != std::string::npos,
            "bridge log lost diagnostic event fields");
        require(log.find("op=runtime_identity result=1") != std::string::npos &&
                log.find("op=presenter_transition result=3") !=
                    std::string::npos &&
                log.find("op=presenter_submission result=0") !=
                    std::string::npos,
            "bridge log is missing V043 presenter event names");
        require(
            log.find(
                "op=nvidia_gpu_stages result=2 dur_us=0 a=101 b=202 c=303") !=
                    std::string::npos &&
                log.find(
                    "op=nvidia_gpu_total result=0 dur_us=0 a=404 b=1010 c=77") !=
                    std::string::npos,
            "bridge log is missing V049 NVIDIA GPU timing fields");

        // Cross several segment boundaries with simultaneous writers. Every
        // event must survive exactly once, including the startup record.
        const auto segmented_dir = temporary.path / "segmented";
        fs::create_directories(segmented_dir);
        write(segmented_dir / "ofxr_bridge.ini",
            "[diagnostics]\nlogging_enabled=1\nmax_file_mb=1\n");
        xrfg::BridgeFlightLogger segmented;
        segmented.initialize(segmented_dir);
        const auto first_path = segmented.log_path();
        std::vector<std::thread> writers;
        for (unsigned worker = 0; worker < 4; ++worker) {
            writers.emplace_back([&, worker] {
                for (unsigned index = 0; index < 10000; ++index) {
                    segmented.event(xrfg::BridgeFlightOperation::synthesis_pair,
                        0, worker, index, 0);
                }
            });
        }
        for (auto& writer : writers) writer.join();
        segmented.shutdown();
        std::set<std::uint64_t> sequences;
        unsigned files = 0;
        for (const auto& item : fs::directory_iterator(segmented_dir)) {
            if (item.path().extension() != ".log") continue;
            ++files;
            require(fs::file_size(item.path()) <= 1024 * 1024,
                "segment exceeded configured limit");
            std::istringstream lines(read(item.path()));
            std::string line;
            while (std::getline(lines, line)) {
                require(line.starts_with("seq="), "incomplete recorder line");
                require(sequences.insert(std::stoull(line.substr(4))).second,
                    "duplicate event across segments");
            }
        }
        require(files > 2 && sequences.size() == 40002 &&
                *sequences.begin() == 1 && *sequences.rbegin() == 40002,
            "rotation lost events");
        require(read(first_path).starts_with("seq=1 "), "startup was overwritten");

        // A collision must preserve both the old segment and future events.
        const auto failure_dir = temporary.path / "rotation-failure";
        fs::create_directories(failure_dir);
        write(failure_dir / "ofxr_bridge.ini",
            "[diagnostics]\nlogging_enabled=1\nmax_file_mb=1\n");
        xrfg::BridgeFlightLogger failure;
        failure.initialize(failure_dir);
        const auto failure_path = failure.log_path();
        auto collision = failure_path;
        collision.replace_extension(L".part1.log");
        write(collision, "preserve existing file");
        for (unsigned index = 0; index < 20000; ++index)
            failure.event(xrfg::BridgeFlightOperation::synthesis_pair, 0, index);
        failure.shutdown();
        const auto retained = read(failure_path);
        require(retained.starts_with("seq=1 ") &&
                retained.find("seq=20002 ") != std::string::npos &&
                read(collision) == "preserve existing file",
            "failed rotation discarded data");

        std::cout << "Bridge flight logger tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Test failure: " << exception.what() << '\n';
        return 1;
    }
}
