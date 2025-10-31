// main.cpp
// --------------------------------
// Main.cpp - ImGui stuff + GLFW
// Password Vault App with ImGui UI
// Credits: aggeloskwn7 (github)
// --------------------------------

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define GLFW_EXPOSE_NATIVE_WIN32

#include <Windows.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>   

#include "vault.h"
#include "crypto.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <shlobj.h>
#include <shellapi.h>



std::string formatDate(std::time_t time) {
    std::tm tm{};
    localtime_s(&tm, &time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string getVaultPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::filesystem::path vaultDir = std::filesystem::path(path) / "PasswordVault";
        std::filesystem::create_directories(vaultDir); 
        return (vaultDir / "vault.dat").string();
    }
    return "vault.dat"; 
}

void DragWindow(GLFWwindow* window)
{
    static bool dragging = false;
    static double lastX = 0.0, lastY = 0.0;

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (!dragging) {
            dragging = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int winX, winY;
            glfwGetWindowPos(window, &winX, &winY);
            glfwSetWindowPos(window, winX + (int)(x - lastX), winY + (int)(y - lastY));
        }
    }
    else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dragging = false;
    }
}

std::string GenerateStrongPassword(int length) {
    const std::string chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*()-_=+[]{}<>?";
    std::string pw;
    pw.reserve(length);
    for (int i = 0; i < length; i++) {
        pw.push_back(chars[rand() % chars.size()]);
    }
    return pw;
}



// Globals
Vault g_vault;
std::string g_master;
bool g_unlocked = false;
bool g_firstRun = false;
std::string g_status;
std::time_t now = std::time(nullptr);
bool showAbout = false;

// Buffers
static char masterBuf[128];
static char siteBuf[128];
static char userBuf[128];
static char passBuf[128];

bool file_exists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);               
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);  // allows transparent background (if supported)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);    

    GLFWwindow* window = glfwCreateWindow(1000, 750, "Password Vault", NULL, NULL);
    HWND hwnd = glfwGetWin32Window(window);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);  

    bool clickThrough = false;



    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    io.FontGlobalScale = 1.05f;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowPadding = ImVec2(20, 20);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.0f); 
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.52f, 0.85f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.60f, 0.95f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.45f, 0.80f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::string vaultPath = getVaultPath();
    g_firstRun = !file_exists(vaultPath);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (GetAsyncKeyState(VK_F1) & 1) {
            clickThrough = !clickThrough;
            SetWindowLong(hwnd, GWL_EXSTYLE,
                exStyle | WS_EX_LAYERED | (clickThrough ? WS_EX_TRANSPARENT : 0));
        }

        if (!g_unlocked) {
            ImVec2 winSize(420, 220);
            ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGui::Begin("Login", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

            ImGui::Text("Welcome to Password Vault");
            ImGui::Spacing();

            if (g_firstRun) {
                ImGui::Text("No vault found. Create a master password:");
                ImGui::InputText("##newpw", masterBuf, sizeof(masterBuf), ImGuiInputTextFlags_Password);

                if (ImGui::Button("Create Vault", ImVec2(-1, 0))) {
                    g_master = masterBuf;
                    g_unlocked = true;
                    g_status = "New vault created.";
                }
            }
            else {
                ImGui::Text("Enter your master password:");
                ImGui::InputText("##masterpw", masterBuf, sizeof(masterBuf), ImGuiInputTextFlags_Password);

                if (ImGui::Button("Unlock", ImVec2(-1, 0))) {
                    if (load_vault(g_vault, vaultPath, masterBuf)) {
                        g_master = masterBuf;
                        g_unlocked = true;
                        g_status = "Vault unlocked.";
                    }
                    else {
                        g_status = "Failed to unlock. Wrong password?";
                    }
                }
            }

            if (!g_status.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%s", g_status.c_str());
            }

            ImGui::End();
        }
        else
        {
            ImGui::SetNextWindowSize(ImVec2(850, 550), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 10)); // nice padding
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);          // rounded corners
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);


            ImGui::Begin("Password Vault", nullptr,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

            float titleBarHeight = 36.0f;

            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.11f, 0.14f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            ImGui::BeginChild("titlebar", ImVec2(0, titleBarHeight), false,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            ImU32 colTop = ImGui::GetColorU32(ImVec4(0.18f, 0.40f, 0.75f, 1.0f));
            ImU32 colBottom = ImGui::GetColorU32(ImVec4(0.12f, 0.25f, 0.55f, 1.0f));
            draw_list->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + titleBarHeight),
                colTop, colTop, colBottom, colBottom);


            ImGui::SetCursorPosY(8);
            ImGui::SetCursorPosX(12);
            ImGui::TextColored(ImVec4(0.80f, 0.85f, 0.95f, 1.0f), "Password Vault");

            float closeButtonWidth = 28.0f;
            ImGui::SameLine(ImGui::GetWindowWidth() - closeButtonWidth - 10);
            ImGui::SetCursorPosY(5);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.10f, 0.10f, 1.0f));
            if (ImGui::Button("X", ImVec2(closeButtonWidth, 24))) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::PopStyleColor(3);

            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    static bool dragging = false;
                    static double lastX = 0.0, lastY = 0.0;

                    if (!dragging)
                    {
                        dragging = true;
                        glfwGetCursorPos(window, &lastX, &lastY);
                    }
                    else
                    {
                        double x, y;
                        glfwGetCursorPos(window, &x, &y);
                        int winX, winY;
                        glfwGetWindowPos(window, &winX, &winY);
                        glfwSetWindowPos(window, winX + static_cast<int>(x - lastX), winY + static_cast<int>(y - lastY));
                    }
                }
                else
                {
                    static bool dragging = false;
                    dragging = false;
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);
            ImGui::Separator();
            ImGui::Spacing();


            ImGui::Text("Total Passwords: %zu", g_vault.entries.size());
            ImGui::SameLine();

            // About button
            if (ImGui::Button("About")) {
                showAbout = true;
            }
            ImGui::SameLine();

            // Check Password button
            static bool showPasswordChecker = false;
            if (ImGui::Button("Check Password")) {
                showPasswordChecker = true;
            }
            ImGui::Spacing();


            if (g_vault.entries.empty()) {
                ImGui::Text("You have no saved passwords.");
            }

            ImGui::InputText("Website", siteBuf, sizeof(siteBuf));
            ImGui::InputText("Username", userBuf, sizeof(userBuf));
            ImGui::InputText("Password", passBuf, sizeof(passBuf), ImGuiInputTextFlags_Password);

            if (ImGui::Button("Add Entry", ImVec2(-1, 0))) {
                g_vault.entries.push_back({ siteBuf, userBuf, passBuf });
                save_vault(g_vault, vaultPath, g_master);
                siteBuf[0] = userBuf[0] = passBuf[0] = '\0';
            }

            ImGui::Separator();

            static char searchBuf[128] = "";
            ImGui::Text("Search:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(250);

            ImGui::InputText("##search", searchBuf, sizeof(searchBuf));

            if (strlen(searchBuf) > 0) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.25f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.35f, 0.35f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

                if (ImGui::SmallButton("X")) {
                    searchBuf[0] = '\0'; 
                }

                ImGui::PopStyleColor(3);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                searchBuf[0] = '\0';
            }



            ImGui::BeginChild("vault_entries", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

            if (ImGui::BeginTable("vault_table", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {

                ImGui::TableSetupColumn("Website");
                ImGui::TableSetupColumn("Username");
                ImGui::TableSetupColumn("Password");
                ImGui::TableSetupColumn("Saved At");
                ImGui::TableSetupColumn("Actions");
                ImGui::TableHeadersRow();

                static std::vector<bool> showPw;

                std::string query = searchBuf;
                std::transform(query.begin(), query.end(), query.begin(), ::tolower);

                static int deleteIndex = -1;

                for (size_t i = 0; i < g_vault.entries.size(); i++) {
                    auto& e = g_vault.entries[i];

                    if (!query.empty()) {
                        std::string siteLower = e.website;
                        std::string userLower = e.username;
                        std::transform(siteLower.begin(), siteLower.end(), siteLower.begin(), ::tolower);
                        std::transform(userLower.begin(), userLower.end(), userLower.begin(), ::tolower);

                        if (siteLower.find(query) == std::string::npos &&
                            userLower.find(query) == std::string::npos)
                            continue;
                    }

                    if (showPw.size() < g_vault.entries.size())
                        showPw.resize(g_vault.entries.size(), false);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(e.website.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(e.username.c_str());

                    ImGui::TableSetColumnIndex(2);
                    if (showPw[i]) ImGui::TextUnformatted(e.password.c_str());
                    else ImGui::Text("********");

                    ImGui::TableSetColumnIndex(3);
                    std::string formatted = formatDate(e.saved_at);
                    ImGui::Text("%s", formatted.c_str());

                    ImGui::TableSetColumnIndex(4);
                    if (ImGui::Button(("Show##" + std::to_string(i)).c_str())) {
                        showPw[i] = !showPw[i];
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Copy##" + std::to_string(i)).c_str())) {
                        glfwSetClipboardString(window, e.password.c_str());
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Delete##" + std::to_string(i)).c_str())) {
                        deleteIndex = static_cast<int>(i);
                        ImGui::OpenPopup("ConfirmDelete");
                    }
                }

                // popup should be outside the loop
                if (ImGui::BeginPopupModal("ConfirmDelete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Are you sure you want to delete this entry?");
                    ImGui::Separator();

                    if (ImGui::Button("Yes", ImVec2(100, 0))) {
                        if (deleteIndex >= 0 && deleteIndex < (int)g_vault.entries.size()) {
                            g_vault.entries.erase(g_vault.entries.begin() + deleteIndex);
                            save_vault(g_vault, vaultPath, g_master);
                        }
                        deleteIndex = -1;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                        deleteIndex = -1;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                ImGui::EndTable();
            }

            ImGui::EndChild();

            ImGui::Separator();
            ImGui::Text("Vault saved at: %s", vaultPath.c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() - 180);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1, 1), "Version 1.1");

            // Password Strength Checker Popup
            if (showPasswordChecker) {
                ImGui::OpenPopup("Password Strength Checker");
            }

            bool popupOpen = ImGui::BeginPopupModal("Password Strength Checker", &showPasswordChecker,
                ImGuiWindowFlags_AlwaysAutoResize);

            if (popupOpen) {
                static char testPassword[128] = "";
                static std::string strengthLabel = "";
                static ImVec4 strengthColor = ImVec4(1, 1, 1, 1);
                static std::string generatedPassword = "";

                ImGui::Text("Enter a password to check its strength:");
                ImGui::InputText("##check_pw", testPassword, sizeof(testPassword));

                if (ImGui::Button("Check Strength", ImVec2(150, 0))) {
                    int score = 0;
                    std::string pw = testPassword;

                    bool hasLower = std::any_of(pw.begin(), pw.end(), ::islower);
                    bool hasUpper = std::any_of(pw.begin(), pw.end(), ::isupper);
                    bool hasDigit = std::any_of(pw.begin(), pw.end(), ::isdigit);
                    bool hasSymbol = std::any_of(pw.begin(), pw.end(), [](unsigned char c) { return std::ispunct(c); });

                    if (pw.length() >= 8) score++;
                    if (pw.length() >= 12) score++;
                    if (hasLower && hasUpper) score++;
                    if (hasDigit) score++;
                    if (hasSymbol) score++;

                    if (score <= 2) {
                        strengthLabel = "Weak";
                        strengthColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                    }
                    else if (score == 3) {
                        strengthLabel = "Medium";
                        strengthColor = ImVec4(0.95f, 0.75f, 0.2f, 1.0f);
                    }
                    else if (score == 4) {
                        strengthLabel = "Strong";
                        strengthColor = ImVec4(0.2f, 0.8f, 0.3f, 1.0f);
                    }
                    else {
                        strengthLabel = "Very Strong";
                        strengthColor = ImVec4(0.1f, 0.9f, 0.4f, 1.0f);
                    }
                }

                if (!strengthLabel.empty()) {
                    ImGui::Spacing();
                    ImGui::Text("Strength: ");
                    ImGui::SameLine();
                    ImGui::TextColored(strengthColor, "%s", strengthLabel.c_str());
                }

                ImGui::Separator();
                ImGui::Text("Need a strong one?");
                if (ImGui::Button("Generate Password", ImVec2(180, 0))) {
                    generatedPassword = GenerateStrongPassword(16);
                    strcpy_s(testPassword, generatedPassword.c_str());
                    strengthLabel.clear();
                }

                if (!generatedPassword.empty()) {
                    ImGui::InputText("Generated", testPassword, sizeof(testPassword));
                    ImGui::SameLine();
                    if (ImGui::Button("Copy")) {
                        glfwSetClipboardString(window, testPassword);
                    }
                }

                ImGui::Spacing();
                if (ImGui::Button("Close", ImVec2(100, 0))) {
                    ImGui::CloseCurrentPopup();
                    generatedPassword.clear();
                    strengthLabel.clear();
                    testPassword[0] = '\0';
                }

                ImGui::EndPopup();
            }

            ImGui::End(); 
            ImGui::PopStyleVar(3);

            // About Window
            if (showAbout) {
                ImGuiViewport* main_viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(main_viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::Begin("About", &showAbout,
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings);

                ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Password Vault v1.1");
                ImGui::Separator();
                ImGui::Text("Made by: aggeloskwn7");
                ImGui::TextColored(ImVec4(0.26f, 0.60f, 0.95f, 1.0f), "Source Code:");
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("github.com/aggeloskwn7/Password-Vault");
                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    draw_list->AddLine(ImVec2(min.x, max.y), ImVec2(max.x, max.y),
                        ImGui::GetColorU32(ImVec4(0.2f, 0.6f, 1.0f, 1.0f)), 1.0f);
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        ShellExecuteA(nullptr, "open",
                            "https://github.com/aggeloskwn7/Password-Vault",
                            nullptr, nullptr, SW_SHOWNORMAL);
                    }
                }

                ImGui::Text("Built on: %s %s", __DATE__, __TIME__);
                ImGui::Spacing();
                ImGui::TextWrapped("A simple local password manager built with C++ and Dear ImGui.");
                ImGui::Spacing();
                if (ImGui::Button("Close")) showAbout = false;
                ImGui::End();
            }
        }


        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // clear
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
