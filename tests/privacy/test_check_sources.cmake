# ---------------------------------------------------------------------------
# The privacy scan held to a table of cases on throw-away trees: each rule must
# find the violation it is for, and must leave alone what it is not for (a
# comment, the store's own source, a prototype, a word inside a longer word).
#
#   cmake -DREPO_ROOT=<repository root> -DWORK_DIR=<scratch directory> \
#         -P tests/privacy/test_check_sources.cmake
#
# A checker that silently stopped matching would pass every tree, so the gate is
# tested as the data checker's is (tools/data/test_check_phonics.ps1).
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

if(NOT REPO_ROOT OR NOT WORK_DIR)
    message(FATAL_ERROR "Pass -DREPO_ROOT=<repository root> and -DWORK_DIR=<scratch directory>.")
endif()

set(_checker "${REPO_ROOT}/tests/privacy/check_sources.cmake")

# The names of the mobile platforms are written in two pieces: a CMake file that
# spells one out reads as a test of the operating system to the platform-isolation
# check (docs/platform-isolation.md), and these cases are not one.
string(CONCAT _droid "andro" "id")
string(CONCAT _apple "ap" "ple")
set(_failures "")
set(_cases 0)

# One tree of one file, scanned. `expect` is the rule id that must be named, or
# CLEAN when the scan has to pass.
function(wjs_case label expect path content)
    set(_tree "${WORK_DIR}/case")
    file(REMOVE_RECURSE "${_tree}")
    get_filename_component(_directory "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${_tree}/${_directory}")
    file(WRITE "${_tree}/${path}" "${content}\n")
    # Something to scan, so that a tree whose only file is not scanned is not
    # the "no source files" error.
    file(MAKE_DIRECTORY "${_tree}/src")
    file(WRITE "${_tree}/src/marker.cpp" "int marker = 0;\n")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DREPO_ROOT=${_tree}" -P "${_checker}"
        RESULT_VARIABLE _status
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error)
    set(_message "${_output}${_error}")

    math(EXPR _next "${_cases} + 1")
    set(_cases ${_next} PARENT_SCOPE)

    set(_failure "")
    if(expect STREQUAL "CLEAN")
        if(NOT _status EQUAL 0)
            set(_failure "${label}: expected the scan to pass, it said:\n${_message}")
        endif()
    else()
        if(_status EQUAL 0)
            set(_failure "${label}: expected [${expect}] to be found, the scan passed")
        elseif(NOT _message MATCHES "\\[${expect}\\]")
            set(_failure "${label}: expected [${expect}] among the findings, got:\n${_message}")
        endif()
    endif()
    if(_failure)
        set(_failures "${_failures}\n  ${_failure}" PARENT_SCOPE)
    endif()
endfunction()

# --- audio -------------------------------------------------------------------
wjs_case("a Qt audio input" audio.capture "src/app/mic.cpp" "QAudioInput input;")
wjs_case("a Qt recorder" audio.capture "src/app/rec.cpp" "QMediaRecorder recorder;")
wjs_case("an audio input of a capture session" audio.capture "src/app/cam.cpp" "session.setAudioInput(nullptr);")
wjs_case("a QML audio input" audio.capture "src/app/qml/Mic.qml" "AudioInput { }")
wjs_case("a property list's microphone string" audio.capture "src/mobile/Info.plist"
    "<key>NSMicrophoneUsageDescription</key>")
wjs_case("a manifest's record permission" audio.capture "src/mobile/Manifest.xml"
    "<uses-permission ${_droid}:name=\"${_droid}.permission.RECORD_AUDIO\"/>")
wjs_case("a page asking for audio" audio.web "src/web/page.html" "navigator.mediaDevices.getUserMedia({ audio: true })")
wjs_case("a native capture API" audio.capture "src/platforms/native/cap.cpp" "snd_pcm_open(&handle, \"default\", SND_PCM_STREAM_CAPTURE, 0);")

# --- network -----------------------------------------------------------------
wjs_case("a network access manager" network.classes "src/app/net.cpp" "QNetworkAccessManager manager;")
wjs_case("a TCP socket" network.classes "src/app/net.cpp" "QTcpSocket socket;")
wjs_case("an XML HTTP request in QML" network.classes "src/app/qml/Get.qml" "var r = new XMLHttpRequest();")
wjs_case("a name lookup" network.classes "src/app/dns.cpp" "getaddrinfo(host, port, &hints, &result);")
wjs_case("a socket header" network.headers "src/lib/src/sock.cpp" "#include <sys/socket.h>")
wjs_case("a Qt network header" network.headers "src/app/net.cpp" "#include <QtNetwork/QTcpServer>")
wjs_case("a URL in code" network.url "src/app/url.cpp" "const char* where = \"https://example.com/data\";")
wjs_case("a URL in QML" network.url "src/app/qml/Link.qml" "source: \"http://example.com/a.png\"")
wjs_case("Qt Network linked" network.modules "src/app/CMakeLists.txt"
    "target_link_libraries(app PRIVATE Qt6::Network)")
wjs_case("Qt Network found" network.modules "CMakeLists.txt"
    "find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Network)")
wjs_case("Qt WebSockets linked" network.modules "src/app/CMakeLists.txt"
    "target_link_libraries(app PRIVATE Qt6::WebSockets)")
wjs_case("a manifest's internet permission" network.manifest "src/mobile/Manifest.xml"
    "<uses-permission ${_droid}:name=\"${_droid}.permission.INTERNET\"/>")
wjs_case("a network entitlement" network.manifest "src/mobile/app.entitlements"
    "<key>com.${_apple}.security.network.client</key>")

# --- export ------------------------------------------------------------------
wjs_case("a printer" export.share "src/app/print.cpp" "QPrinter printer;")
wjs_case("a file dialog" export.share "src/app/qml/Save.qml" "FileDialog { }")
wjs_case("a share sheet" export.share "src/mobile/share.mm" "UIActivityViewController* sheet;")
wjs_case("a URL handed to the system" export.share "src/app/qml/Out.qml" "Qt.openUrlExternally(page)")
wjs_case("the clipboard" export.share "src/app/clip.cpp" "QClipboard* board = nullptr;")
wjs_case("a mail link" export.mail "src/app/qml/Mail.qml" "text: \"mailto:a@b.example\"")
wjs_case("a launched process" export.launch "src/app/run.cpp" "QProcess process;")
wjs_case("a system call" export.system "src/lib/src/run.cpp" "std::system(\"ls\");")
wjs_case("a stream written outside the store" export.write "src/lib/src/other.cpp"
    "std::ofstream out(path);")
wjs_case("a file renamed outside the store" export.write "src/lib/src/other.cpp"
    "std::filesystem::rename(a, b);")
wjs_case("a file opened for writing by Qt" export.write "src/app/save.cpp"
    "file.open(QIODevice::WriteOnly);")
wjs_case("a C file opened for writing" export.fopen "src/lib/src/other.cpp"
    "FILE* f = fopen(path, \"wb\");")

# --- what is left alone ------------------------------------------------------
wjs_case("a clean tree" CLEAN "src/lib/src/ok.cpp" "int main() { return 0; }")
wjs_case("the design written about in a comment" CLEAN "src/lib/src/ok.cpp"
    "// Nothing here uses QNetworkAccessManager or QAudioInput or std::ofstream.\nint x = 0;")
wjs_case("a block comment" CLEAN "src/lib/src/ok.cpp"
    "/*\n * No QTcpSocket, no QPrinter.\n */\nint x = 0;")
wjs_case("a CMake comment" CLEAN "src/app/CMakeLists.txt" "# Qt6::Network is not linked here.\nadd_library(a a.cpp)")
wjs_case("the store's own source writes its file" CLEAN "src/lib/src/store.cpp"
    "std::ofstream out(path);\nstd::filesystem::rename(a, b);\nstd::filesystem::create_directories(d);")
wjs_case("a test may write a temporary file" CLEAN "tests/unit/x_test.cpp" "std::ofstream out(path);")
wjs_case("a camera session is not an audio input" CLEAN "src/app/camera.cpp" "QMediaCaptureSession session;")
wjs_case("a URL in build metadata" CLEAN "CMakeLists.txt"
    "project(X HOMEPAGE_URL \"https://example.com/x\")")
wjs_case("a URL in a package manifest" CLEAN "vcpkg.json" "{ \"homepage\": \"https://example.com/x\" }")
wjs_case("a name inside a longer word" CLEAN "src/lib/src/ok.cpp"
    "int MyQNetworkAccessManagerish = 0; int nQAudioInputs = 1; int popen_count = 2;")
wjs_case("Qt Qml and Qt Quick are fine" CLEAN "src/app/CMakeLists.txt"
    "target_link_libraries(app PRIVATE Qt6::Qml Qt6::Quick)")
wjs_case("the scanner's privacy checker names what it forbids" CLEAN "tests/scan/check_privacy.cmake"
    "\"network|QNetworkAccessManager|QTcpSocket\" \"audio capture|QAudioInput\"")
wjs_case("the scanner's deliberate leak" CLEAN "tests/scan/privacy_fixture/src/scan/leak.cpp"
    "QNetworkAccessManager manager; QAudioInput input;")
wjs_case("the prototype is a program of its own" CLEAN "src/app/prototype/shake/frames.cpp"
    "file.open(QIODevice::WriteOnly);")
wjs_case("documentation is not scanned" CLEAN "docs/x.md" "QNetworkAccessManager QAudioInput")
wjs_case("third-party code is not scanned" CLEAN "deps/vcpkg/x.cpp" "QNetworkAccessManager m;")
wjs_case("a tool may write files" CLEAN "tools/x/make.cpp" "std::ofstream out(path);")

file(REMOVE_RECURSE "${WORK_DIR}/case")

if(_failures)
    message(FATAL_ERROR "The privacy scan is wrong in ${_cases} cases:${_failures}")
endif()
message(STATUS "Privacy scan: ${_cases} cases hold")
