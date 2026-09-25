// Implements the ImGuiLayer class that integrates Dear ImGui for rendering the user interface
#pragma once

struct GLFWwindow;

class ImGuiLayer
{
public:
    explicit ImGuiLayer(GLFWwindow* window);
    ~ImGuiLayer();

    void BeginFrame();
    void EndFrame();
};