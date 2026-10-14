// clang-format off: Windows declarations must precede the common-control and shell headers.
#include <windows.h>

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
// clang-format on

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "kazarov/data_version.hpp"
#include "portable.hpp"

namespace {
namespace fs = std::filesystem;
constexpr UINT Finished = WM_APP + 1, GameStarting = WM_APP + 2;
constexpr int  Browse = 101, Prepare = 102, Logs = 103, Source = 104;

struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;

    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}

    ~Handle() {
        if (value && value != INVALID_HANDLE_VALUE) {
            CloseHandle(value);
        }
    }

    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
};

std::wstring Wide(const std::string &text) {
    if (text.empty()) {
        return {};
    }
    const int    length = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), result.data(), length);
    return result;
}

std::string WinError(const char *action) {
    const DWORD code = GetLastError();
    wchar_t    *message = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t *>(&message), 0, nullptr);
    std::string result = std::string(action) + " (Windows error " + std::to_string(code) + ").";
    if (message) {
        const int   length = WideCharToMultiByte(CP_UTF8, 0, message, -1, nullptr, 0, nullptr, nullptr);
        std::string detail(length, '\0');
        WideCharToMultiByte(CP_UTF8, 0, message, -1, detail.data(), length, nullptr, nullptr);
        if (!detail.empty()) {
            detail.pop_back();
        }
        result += " " + detail;
        LocalFree(message);
    }
    return result;
}

fs::path PackageDirectory() {
    std::vector<wchar_t> path(512);
    for (;;) {
        const auto length = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
        if (!length) {
            throw std::runtime_error(WinError("Could not locate the launcher"));
        }
        if (length < path.size()) {
            return fs::path(std::wstring(path.data(), length)).parent_path();
        }
        path.resize(path.size() * 2);
    }
}

DWORD RunProcess(const fs::path &program, const std::vector<std::wstring> &arguments,
                 const fs::path &workingDirectory, const fs::path &log, HANDLE portableLock = nullptr) {
    if (fs::exists(log)) {
        auto previous = log;
        previous.replace_filename(log.stem().wstring() + L".previous.log");
        if (!MoveFileExW(log.c_str(), previous.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            throw std::runtime_error(WinError("Could not preserve the previous log"));
        }
    }
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    Handle              output(CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle              input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (output.value == INVALID_HANDLE_VALUE || input.value == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(WinError("Could not create the game log"));
    }
    Handle inheritedLock;
    if (portableLock && !DuplicateHandle(GetCurrentProcess(), portableLock, GetCurrentProcess(),
                                         &inheritedLock.value, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
        throw std::runtime_error(WinError("Could not protect the portable game folder"));
    }
    Handle                               job(portableLock ? nullptr : CreateJobObjectW(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!portableLock && (!job.value || !SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))) {
        throw std::runtime_error(WinError("Could not prepare the game process"));
    }
    // Inherit only the redirected streams and (for the game) the folder lock.
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    std::vector<BYTE> attributeStorage(attributeBytes);
    auto              attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
    if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes)) {
        throw std::runtime_error(WinError("Could not prepare process handles"));
    }

    struct Attributes {
        LPPROC_THREAD_ATTRIBUTE_LIST value;

        ~Attributes() { DeleteProcThreadAttributeList(value); }
    } attributeCleanup{attributes};

    std::vector<HANDLE> handles{input.value, output.value};
    if (portableLock) {
        handles.push_back(inheritedLock.value);
    }
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles.data(),
                                   handles.size() * sizeof(HANDLE), nullptr, nullptr)) {
        throw std::runtime_error(WinError("Could not prepare process handles"));
    }
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input.value;
    startup.StartupInfo.hStdOutput = startup.StartupInfo.hStdError = output.value;
    startup.lpAttributeList = attributes;
    PROCESS_INFORMATION process{};
    auto                command = portable::CommandLine(program, arguments);
    if (!CreateProcessW(program.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
                        nullptr, workingDirectory.c_str(), &startup.StartupInfo, &process)) {
        throw std::runtime_error(WinError("Could not start the bundled program"));
    }
    Handle processHandle(process.hProcess), threadHandle(process.hThread);
    if (!portableLock && !AssignProcessToJobObject(job.value, processHandle.value)) {
        const auto error = WinError("Could not manage the bundled program");
        TerminateProcess(processHandle.value, 1);
        throw std::runtime_error(error);
    }
    if (ResumeThread(threadHandle.value) == DWORD(-1)) {
        const auto error = WinError("Could not start the bundled program");
        TerminateProcess(processHandle.value, 1);
        throw std::runtime_error(error);
    }
    if (WaitForSingleObject(processHandle.value, INFINITE) != WAIT_OBJECT_0) {
        throw std::runtime_error(WinError("Could not wait for the bundled program"));
    }
    DWORD exitCode = 1;
    if (!GetExitCodeProcess(processHandle.value, &exitCode)) {
        throw std::runtime_error(WinError("Could not read the program's result"));
    }
    return exitCode;
}

fs::path PickIso(HWND owner, const fs::path &suggested) {
    std::vector<wchar_t> selected(32768);
    const auto           initial = suggested.empty() ? fs::path() : suggested.parent_path();
    OPENFILENAMEW        picker{};
    picker.lStructSize = sizeof(picker);
    picker.hwndOwner = owner;
    picker.lpstrFilter = L"ISO disc images (*.iso)\0*.iso\0\0";
    picker.lpstrFile = selected.data();
    picker.nMaxFile = DWORD(selected.size());
    picker.lpstrTitle = L"Choose your PAL Dark Chronicle ISO";
    picker.lpstrInitialDir = initial.empty() ? nullptr : initial.c_str();
    picker.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_DONTADDTORECENT;
    if (GetOpenFileNameW(&picker)) {
        return fs::path(selected.data());
    }
    if (CommDlgExtendedError()) {
        throw std::runtime_error("The file picker could not open. Paste the full ISO path into setup instead.");
    }
    return {};
}

struct Options {
    fs::path                  iso;
    bool                      quiet = false, prepareOnly = false;
    std::vector<std::wstring> gameArguments;
};

Options Parse(const std::vector<std::wstring> &arguments) {
    Options options;
    for (std::size_t i = 1; i < arguments.size(); ++i) {
        const auto &argument = arguments[i];
        if (argument == L"--no-ui") {
            options.quiet = true;
        } else if (argument == L"--prepare-only") {
            options.prepareOnly = true;
        } else if (argument == L"--iso") {
            if (i + 1 == arguments.size()) {
                throw std::runtime_error("--iso requires a disc image path.");
            }
            options.iso = fs::absolute(arguments[++i]);
        } else if (argument == L"--") {
            options.gameArguments.assign(arguments.begin() + i + 1, arguments.end());
            for (const auto &gameArgument : options.gameArguments) {
                for (const auto *reserved : {L"--data", L"--save", L"--run-dir"}) {
                    if (gameArgument == reserved || gameArgument.starts_with(std::wstring(reserved) + L"=")) {
                        throw std::runtime_error("Play.exe chooses the portable data and save location. Launch darkchronicle.exe directly to choose another location.");
                    }
                }
            }
            break;
        } else {
            throw std::runtime_error("Unknown launcher option. Run Play.exe without options to choose your ISO and play.");
        }
    }
    return options;
}

struct App {
    fs::path    package, root, iso;
    Options     options;
    HWND        window{}, heading{}, description{}, source{}, browse{}, status{}, progress{}, prepare{}, logs{}, cancel{};
    HFONT       font{}, titleFont{};
    bool        busy = false;
    bool        choosing = false;
    DWORD       result = 0;
    std::string error;
    std::thread worker;
    HANDLE      portableLock{};

    ~App() {
        if (worker.joinable()) {
            worker.join();
        }
        if (font) {
            DeleteObject(font);
        }
        if (titleFont) {
            DeleteObject(titleFont);
        }
    }

    void Layout() {
        const auto dpi = GetDpiForWindow(window);
        auto       px = [dpi](int value) { return MulDiv(value, dpi, 96); };
        if (font) {
            DeleteObject(font);
        }
        if (titleFont) {
            DeleteObject(titleFont);
        }
        font = CreateFontW(-MulDiv(10, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        titleFont = CreateFontW(-MulDiv(18, dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        const struct {
            HWND control;
            int  x, y, width, height;
        } controls[]{
            {heading,     24,  20,  572, 36},
            {description, 24,  65,  572, 46},
            {source,      24,  124, 444, 28},
            {browse,      480, 123, 116, 30},
            {status,      24,  168, 572, 56},
            {progress,    24,  232, 572, 8 },
            {logs,        24,  258, 100, 32},
            {prepare,     316, 258, 174, 32},
            {cancel,      502, 258, 94,  32}
        };

        for (const auto &control : controls) {
            MoveWindow(control.control, px(control.x), px(control.y), px(control.width), px(control.height), TRUE);
            SendMessageW(control.control, WM_SETFONT, WPARAM(control.control == heading ? titleFont : font), TRUE);
        }
    }

    void Refresh() {
        SetWindowTextW(source, iso.c_str());
        EnableWindow(prepare, !busy && !iso.empty());
        EnableWindow(source, !busy);
        EnableWindow(browse, !busy);
        EnableWindow(cancel, !busy);
        SendMessageW(progress, PBM_SETMARQUEE, busy, 30);
        ShowWindow(progress, busy ? SW_SHOW : SW_HIDE);
    }

    void Run() {
        try {
            if (!portable::CurrentData(root, KAZAROV_DATA_VERSION)) {
                if (iso.empty()) {
                    throw std::runtime_error("Choose your PAL Dark Chronicle ISO first.");
                }
                portable::CheckIso(iso);
                const auto code = RunProcess(package / "dc2data.exe", {L"extract", iso.wstring(), (root / "data").wstring()},
                                             package, root / "logs" / "setup.log");
                if (code || !portable::CurrentData(root, KAZAROV_DATA_VERSION)) {
                    throw std::runtime_error("Game files could not be prepared. Open logs for the extraction error, check your ISO and available disk space, then try again.");
                }
                portable::RememberIso(package, iso);
            }
            if (options.prepareOnly) {
                result = 0;
                return;
            }
            if (window) {
                PostMessageW(window, GameStarting, 0, 0);
            }
            auto arguments = std::vector<std::wstring>{L"--run-dir", root.wstring()};
            arguments.insert(arguments.end(), options.gameArguments.begin(), options.gameArguments.end());
            result = RunProcess(package / "darkchronicle.exe", arguments, package, root / "logs" / "game.log", portableLock);
            if (result) {
                error = "The game closed with an error (exit " + std::to_string(result) + "). See userdata/logs/game.log for details.";
            }
        } catch (const std::exception &failure) {
            error = failure.what();
            result = 1;
        }
    }

    void Start() {
        busy = true;
        error.clear();
        SetWindowTextW(status, L"Preparing game files. This can take several minutes the first time.\r\nThe game will open automatically when setup finishes.");
        Refresh();
        try {
            worker = std::thread([this] { Run(); PostMessageW(window, Finished, 0, 0); });
        } catch (const std::exception &failure) {
            busy = false;
            result = 1;
            SetWindowTextW(status, Wide(failure.what()).c_str());
            Refresh();
        }
    }
};

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto *app = reinterpret_cast<App *>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
        app->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, LONG_PTR(app));
    }
    if (!app) {
        return DefWindowProcW(window, message, wparam, lparam);
    }
    switch (message) {
        case WM_CREATE: {
            auto control = [window](const wchar_t *kind, const wchar_t *text, DWORD style, int id = 0) {
                return CreateWindowExW(kind == std::wstring(L"EDIT") ? WS_EX_CLIENTEDGE : 0, kind, text,
                                       WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, window,
                                       reinterpret_cast<HMENU>(INT_PTR(id)), GetModuleHandleW(nullptr), nullptr);
            };
            app->heading = control(L"STATIC", L"Set up Dark Chronicle", 0);
            app->description = control(L"STATIC", L"Choose your PAL European Dark Chronicle ISO.\r\nData and saves stay in this folder; later launches go straight to the game.", 0);
            app->source = control(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, Source);
            SendMessageW(app->source, EM_SETCUEBANNER, TRUE, LPARAM(L"Choose an ISO or paste its full path"));
            app->browse = control(L"BUTTON", L"Choose ISO…", BS_PUSHBUTTON | WS_TABSTOP, Browse);
            app->status = control(L"STATIC", app->iso.empty() ? L"Select a disc image to begin." : L"A PAL Dark Chronicle disc image was found. Ready to prepare game files.", 0);
            app->progress = control(PROGRESS_CLASSW, L"", PBS_MARQUEE);
            app->logs = control(L"BUTTON", L"Open logs", BS_PUSHBUTTON | WS_TABSTOP, Logs);
            app->prepare = control(L"BUTTON", L"Prepare and play", BS_DEFPUSHBUTTON | WS_TABSTOP, Prepare);
            app->cancel = control(L"BUTTON", L"Close", BS_PUSHBUTTON | WS_TABSTOP, IDCANCEL);
            app->Layout();
            app->Refresh();
            SetFocus(app->iso.empty() ? app->browse : app->prepare);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wparam) == Source && HIWORD(wparam) == EN_CHANGE && !app->busy) {
                std::wstring path(GetWindowTextLengthW(app->source) + 1, L'\0');
                GetWindowTextW(app->source, path.data(), int(path.size()));
                path.resize(wcslen(path.c_str()));
                const auto first = path.find_first_not_of(L" \t\r\n");
                if (first == std::wstring::npos) {
                    path.clear();
                } else {
                    path = path.substr(first, path.find_last_not_of(L" \t\r\n") - first + 1);
                    if (path.size() >= 2 && path.front() == L'"' && path.back() == L'"') {
                        path = path.substr(1, path.size() - 2);
                    }
                }
                app->iso = fs::path(path);
                if (!app->iso.empty() && app->iso.is_relative()) {
                    app->iso = app->package / app->iso;
                }
                EnableWindow(app->prepare, !app->iso.empty());
            } else if (LOWORD(wparam) == Browse && !app->busy && !app->choosing) {
                app->choosing = true;
                try {
                    const auto selected = PickIso(window, app->iso);
                    app->choosing = false;
                    if (!selected.empty()) {
                        portable::CheckIso(selected);
                        app->iso = selected;
                        SetWindowTextW(app->status, L"PAL Dark Chronicle disc selected. Ready to prepare game files.");
                        app->Refresh();
                        SetFocus(app->prepare);
                    }
                } catch (const std::exception &error) {
                    app->choosing = false;
                    SetWindowTextW(app->status, Wide(error.what()).c_str());
                    MessageBoxW(window, Wide(error.what()).c_str(), L"Choose another disc image", MB_OK | MB_ICONWARNING);
                }
            } else if ((LOWORD(wparam) == Prepare || LOWORD(wparam) == IDOK) && !app->busy && !app->choosing && !app->iso.empty()) {
                app->Start();
            } else if (LOWORD(wparam) == Logs) {
                ShellExecuteW(window, L"open", (app->root / "logs").c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            } else if (LOWORD(wparam) == IDCANCEL && !app->busy && !app->choosing) {
                DestroyWindow(window);
            }
            return 0;
        case WM_CTLCOLORSTATIC:
            SetBkColor(reinterpret_cast<HDC>(wparam), GetSysColor(COLOR_WINDOW));
            SetTextColor(reinterpret_cast<HDC>(wparam), GetSysColor(COLOR_WINDOWTEXT));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        case GameStarting:
            ShowWindow(window, SW_HIDE);
            return 0;
        case Finished:
            app->worker.join();
            app->busy = false;
            if (app->error.empty()) {
                DestroyWindow(window);
            } else {
                ShowWindow(window, SW_SHOW);
                SetWindowTextW(app->status, Wide(app->error).c_str());
                app->Refresh();
                SetWindowTextW(app->prepare, portable::CurrentData(app->root, KAZAROV_DATA_VERSION) ? L"Play again" : L"Try again");
                SetFocus(app->logs);
            }
            return 0;
        case WM_DPICHANGED: {
            const auto *bounds = reinterpret_cast<RECT *>(lparam);
            SetWindowPos(window, nullptr, bounds->left, bounds->top, bounds->right - bounds->left,
                         bounds->bottom - bounds->top, SWP_NOZORDER | SWP_NOACTIVATE);
            app->Layout();
            return 0;
        }
        case WM_CLOSE:
            if (app->choosing) {
                return 0;
            }
            if (app->busy) {
                MessageBoxW(window, L"Game files are still being prepared. Leave setup open until it finishes.",
                            L"Setup in progress", MB_OK | MB_ICONINFORMATION);
            } else {
                DestroyWindow(window);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(int(app->result));
            return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int Run(const std::vector<std::wstring> &arguments) {
    if (arguments.size() == 3 && arguments[1] == L"--check-iso") {
        try {
            portable::CheckIso(arguments[2]);
            std::puts("PAL Dark Chronicle (SCES-51190): OK");
            return 0;
        } catch (const std::exception &error) {
            std::fprintf(stderr, "%s\n", error.what());
            return 1;
        }
    }
    App app;
    app.options = Parse(arguments);
    app.package = PackageDirectory();
    app.root = app.package / "userdata";
    for (const auto *name : {L"dc2data.exe", L"darkchronicle.exe"}) {
        if (!fs::is_regular_file(app.package / name)) {
            throw std::runtime_error("The portable package is incomplete. Extract the entire ZIP into a folder, then run Play.exe there.");
        }
    }
    try {
        fs::create_directories(app.root / "logs");
    } catch (const fs::filesystem_error &) {
        throw std::runtime_error("This folder cannot store game files and saves. Move the portable folder to a writable location, then reopen Play.exe.");
    }
    Handle lock(CreateFileW((app.root / ".portable.lock").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr));
    if (lock.value == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_SHARING_VIOLATION) {
            throw std::runtime_error("ChronicleTwo is already running or preparing files from this folder. Close it before starting another copy.");
        }
        throw std::runtime_error("This folder is not writable. Move the portable folder to a writable location, then reopen Play.exe.");
    }
    app.portableLock = lock.value;
    app.iso = app.options.iso;
    const bool current = portable::CurrentData(app.root, KAZAROV_DATA_VERSION);
    if (current || app.options.quiet) {
        app.Run();
        if (!app.error.empty()) {
            std::fprintf(stderr, "%s\n", app.error.c_str());
            if (!app.options.quiet) {
                MessageBoxW(nullptr, Wide(app.error).c_str(), L"ChronicleTwo", MB_OK | MB_ICONERROR);
            }
        }
        return int(app.result);
    }
    if (app.iso.empty()) {
        app.iso = portable::SuggestedIso(app.package);
    } else {
        portable::CheckIso(app.iso);
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&controls);
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = L"ChronicleTwoPortable";
    if (!RegisterClassW(&windowClass)) {
        throw std::runtime_error(WinError("Could not open setup"));
    }
    const auto  dpi = GetDpiForSystem();
    RECT        bounds{0, 0, MulDiv(620, dpi, 96), MulDiv(314, dpi, 96)};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectExForDpi(&bounds, style, FALSE, 0, dpi);
    const auto window = CreateWindowExW(0, windowClass.lpszClassName, L"ChronicleTwo", style,
                                        CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
                                        nullptr, nullptr, windowClass.hInstance, &app);
    if (!window) {
        throw std::runtime_error(WinError("Could not open setup"));
    }
    ShowWindow(window, SW_SHOWNORMAL);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return int(app.result);
}
} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int        count = 0;
    auto     **parsed = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!parsed) {
        return 1;
    }
    std::vector<std::wstring> arguments(parsed, parsed + count);
    LocalFree(parsed);
    const bool quiet = std::find(arguments.begin(), arguments.end(), L"--no-ui") != arguments.end();
    int        result = 1;
    try {
        result = Run(arguments);
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        if (!quiet) {
            MessageBoxW(nullptr, Wide(error.what()).c_str(), L"ChronicleTwo", MB_OK | MB_ICONERROR);
        }
    }
    if (SUCCEEDED(initialized)) {
        CoUninitialize();
    }
    return result;
}
