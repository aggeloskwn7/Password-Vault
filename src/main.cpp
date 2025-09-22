// main.cpp
// --------------------------------
// Main.cpp - entry point
// Password Vault App with ImGui UI
// Credits: aggeloskwn7 (github)
// --------------------------------

#include "vault.h"
#include "crypto.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

std::string getVaultPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::filesystem::path vaultDir = std::filesystem::path(path) / "PasswordVault";
        std::filesystem::create_directories(vaultDir); // ensure folder exists
        return (vaultDir / "vault.dat").string();
    }
    return "vault.dat"; // fallback if something goes wrong
}


// Globals
Vault g_vault;
std::string g_master;
bool g_unlocked = false;
bool g_firstRun = false;
std::string g_status;

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
    // Init GLFW
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(900, 600, "Password Vault", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Load modern font
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

    // Apply style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowPadding = ImVec2(20, 20);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.0f); // dark gray window
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.52f, 0.85f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.60f, 0.95f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.45f, 0.80f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Detect first run
    std::string vaultPath = getVaultPath();
    g_firstRun = !file_exists(vaultPath);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!g_unlocked) {
            // === Login card ===
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
        else {
            // === Vault window ===
            ImGui::SetNextWindowSize(ImVec2(850, 550), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

            ImGui::Begin("Password Vault", nullptr,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

            ImGui::Text("Entries: %zu", g_vault.entries.size());
            ImGui::Spacing();

            if (g_vault.entries.empty()) {
                ImGui::Text("You have no saved passwords.");
            }

            // Add entry form
            ImGui::InputText("Website", siteBuf, sizeof(siteBuf));
            ImGui::InputText("Username", userBuf, sizeof(userBuf));
            ImGui::InputText("Password", passBuf, sizeof(passBuf), ImGuiInputTextFlags_Password);

            if (ImGui::Button("Add Entry", ImVec2(-1, 0))) {
                g_vault.entries.push_back({ siteBuf, userBuf, passBuf });
                save_vault(g_vault, vaultPath, g_master);
                siteBuf[0] = userBuf[0] = passBuf[0] = '\0';
            }

            ImGui::Separator();

            // Scrollable child region for entries
            ImGui::BeginChild("vault_entries", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

            // Table of entries
            if (ImGui::BeginTable("vault_table", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {

                ImGui::TableSetupColumn("Website");
                ImGui::TableSetupColumn("Username");
                ImGui::TableSetupColumn("Password");
                ImGui::TableSetupColumn("Actions");
                ImGui::TableHeadersRow();

                static std::vector<bool> showPw;

                for (size_t i = 0; i < g_vault.entries.size(); i++) {
                    auto& e = g_vault.entries[i];
                    if (showPw.size() < g_vault.entries.size())
                        showPw.resize(g_vault.entries.size(), false);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(e.website.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(e.username.c_str());

                    ImGui::TableSetColumnIndex(2);
                    if (showPw[i]) ImGui::TextUnformatted(e.password.c_str());
                    else ImGui::Text("********");

                    ImGui::TableSetColumnIndex(3);
                    if (ImGui::Button(("Show##" + std::to_string(i)).c_str())) {
                        showPw[i] = !showPw[i];
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("Copy##" + std::to_string(i)).c_str())) {
                        glfwSetClipboardString(window, e.password.c_str());
                    }
                }
                ImGui::EndTable();
            }

            ImGui::EndChild();
            ImGui::End();
        }
        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.10f, 0.11f, 0.13f, 1.0f); // dark background
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
