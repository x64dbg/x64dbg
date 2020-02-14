// ImGui - standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <imgui_memory_editor/imgui_memory_editor.h>

#include <stdio.h>
#include <algorithm>

// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#include <GLFW/glfw3.h> // Include glfw3.h after our OpenGL definitions

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static bool glfwSetWindowCenter(GLFWwindow* window)
{
    if(!window)
        return false;

    int sx = 0, sy = 0;
    int px = 0, py = 0;
    int mx = 0, my = 0;
    int monitor_count = 0;
    int best_area = 0;
    int final_x = 0, final_y = 0;

    glfwGetWindowSize(window, &sx, &sy);
    glfwGetWindowPos(window, &px, &py);

    // Iterate throug all monitors
    GLFWmonitor** m = glfwGetMonitors(&monitor_count);
    if(!m)
        return false;

    for(int j = 0; j < monitor_count; ++j)
    {

        glfwGetMonitorPos(m[j], &mx, &my);
        const GLFWvidmode* mode = glfwGetVideoMode(m[j]);
        if(!mode)
            continue;

        // Get intersection of two rectangles - screen and window
        int minX = std::max(mx, px);
        int minY = std::min(my, py);

        int maxX = std::min(mx + mode->width, px + sx);
        int maxY = std::min(my + mode->height, py + sy);

        // Calculate area of the intersection
        int area = std::max(maxX - minX, 0) * std::max(maxY - minY, 0);

        // If its bigger than actual (window covers more space on this monitor)
        if(area > best_area)
        {
            // Calculate proper position in this monitor
            final_x = mx + (mode->width - sx) / 2;
            final_y = my + (mode->height - sy) / 2;

            best_area = area;
        }

    }

    // We found something
    if(best_area)
        glfwSetWindowPos(window, final_x, final_y);

    // Something is wrong - current window has NOT any intersection with any monitors. Move it to the default one.
    else
    {
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        if(primary)
        {
            const GLFWvidmode* desktop = glfwGetVideoMode(primary);

            if(desktop)
                glfwSetWindowPos(window, (desktop->width - sx) / 2, (desktop->height - sy) / 2);
            else
                return false;
        }
        else
            return false;
    }

    return true;
}

#include <string>

std::string readAllText(const char* file)
{
    std::string result;
    HANDLE hFile = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        result.resize(GetFileSize(hFile, nullptr));
        DWORD read;
        if(!ReadFile(hFile, (void*)result.data(), result.size(), &read, nullptr))
            result.clear();
    }
    return result;
}

const char* stristr(const char* String, const char* Pattern)
{
    const char* pptr, *sptr, *start;

    for(start = (char*)String; *start; start++)
    {
        /* find start of pattern in string */
        for(; (*start && (toupper(*start) != toupper(*Pattern))); start++)
            ;
        if(!*start)
            return 0;

        pptr = (char*)Pattern;
        sptr = (char*)start;

        while(toupper(*sptr) == toupper(*pptr))
        {
            sptr++;
            pptr++;
            /* if end of pattern then pattern was found */
            if(!*pptr)
                return (start);
        }
    }
    return 0;
}

static void ShowVirtualScrollingView(bool* p_open)
{
    const uint32_t start_address = 0x400000;
    static uint32_t current_address = start_address;
    static uint32_t end_address = start_address + 1024;

    ImGui::SetNextWindowSize(ImVec2(500, 500));
    if(!ImGui::Begin("Example: Virtual Scrolling", p_open))
    {
        ImGui::End();
        return;
    }

    // Set up some constants for the drawing

    const float font_size = ImGui::GetFontSize();
    const ImVec2 window_size = ImGui::GetWindowSize();
    // TODO: Proper calcutalition of chars per row
    int drawable_chars = (int)(window_size.x / (font_size * 2.3f));
    int drawable_line_count = (int)((end_address - start_address) / drawable_chars);

    //printf("%d\n", drawable_line_count);

    uint32_t address = current_address;
    float scroll = ImGui::GetScrollY();

    current_address = start_address + scroll * 2;
    end_address = current_address + 1024;

    printf("0x%x 0x%x (%d)\n", current_address, end_address, drawable_line_count);

    ImGuiListClipper clipper(drawable_line_count);

    while(clipper.Step())
    {
        for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            ImGui::Text("0x%08x: ", address);
            ImGui::SameLine(0, -1);

            // Print hex values

            for(int p = 0; p < drawable_chars; ++p)
            {
                uint32_t c = (address * (address + p)) & 0xff;
                ImGui::Text("%02x", c);
                ImGui::SameLine(0, -1);
            }

            // print characters

            for(int p = 0; p < drawable_chars; ++p)
            {
                uint32_t c = (address * (address + p)) & 0xff;
                uint8_t wc = 0;

                if(c >= 32 && c < 128)
                    wc = (char)c;
                else
                    wc = '.';

                ImGui::Text("%c", wc);
                ImGui::SameLine(0, 0);
            }

            ImGui::Text("\n");

            address += drawable_chars;
        }
    }

    ImGui::End();
}

#include <vector>
int xxxmain(int, char**)
{
    struct BanEntry
    {
        std::string name;
        std::string status;
    };
    std::vector<BanEntry> banlist, filteredBanlist;
    filteredBanlist = banlist;

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1800, 950, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if(window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() != 0;
#endif
    if(err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO & io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    //ImGui::StyleColorsLight();
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while(!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        if(ImGui::IsKeyReleased(ImGuiKey_Escape))
            break;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static MemoryEditor memoryEditor;
        static std::vector<unsigned char> memData;
        memData.resize(0x10000);
        memoryEditor.DrawWindow("MemoryEditor", memData.data(), memData.size());

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if(show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        if(false)
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if(ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if(show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if(ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // banlist
        {
            ImGui::Begin("Yu-Gi-Oh! BANLIST");

            char filter[256];
            if(ImGui::InputText("", filter, _countof(filter)))
            {
                filteredBanlist.clear();
                for(size_t i = 0; i < banlist.size(); i++)
                {
                    auto & ban = banlist.at(i);
                    if(!filter[0] || stristr(ban.name.c_str(), filter) || stristr(ban.status.c_str(), filter))
                        filteredBanlist.push_back(banlist[i]);
                }
            }
            if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
                ImGui::SetKeyboardFocusHere(0);

            ImGui::BeginChild("##ScrollingRegion");
            ImGui::Columns(2);
            ImGuiListClipper clipper(filteredBanlist.size());  // Also demonstrate using the clipper for large list
            while(clipper.Step())
            {
                for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    auto & entry = filteredBanlist.at(i);
                    ImGui::Text(entry.name.c_str());
                    ImGui::NextColumn();
                    ImGui::Text(entry.status.c_str());
                    ImGui::NextColumn();
                }
            }
            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
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
