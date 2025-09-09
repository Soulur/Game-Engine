cmake_minimum_required(VERSION 3.28) # FetchContent is available in 3.11+
project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
find_package(OpenGL REQUIRED)

# 设置Assimp的安装路径
set(Assimp_DIR ${PROJECT_SOURCE_DIR}/vendor/Assimp/lib/cmake/assimp-5.4)
find_package(Assimp REQUIRED)

# 检查是否找到了Assimp
if(Assimp_FOUND)
    message(STATUS "Found Assimp: ${Assimp_DIR}")
else()
    message(FATAL_ERROR "Could not find Assimp")
endif()

# 在顶层 CMakeLists.txt 中设置全局包含路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# =============================================

# Our Project
add_executable(${PROJECT_NAME}
    src/main.cpp

    # Client
    src/Client/ClientApp.cpp
    src/Client/EditorLayer.cpp
    src/Client/EditorLayer.h

    # Core
    src/Core/Base.h
    src/Core/Timer.h
    src/Core/Timestep.h

    src/Core/Application.cpp
    src/Core/Application.h
    
    src/Core/Input.cpp
    src/Core/Input.h

    src/Core/Log.cpp
    src/Core/Log.h

    src/Core/Layer.cpp
    src/Core/Layer.h

    src/Core/LayerStack.cpp
    src/Core/LayerStack.h

    src/Core/Window.cpp
    src/Core/Window.h

    src/Core/UUID.cpp
    src/Core/UUID.h

    # ImGui    
    src/ImGui/ImGuiLayer.cpp
    src/ImGui/ImGuiLayer.h
    src/ImGui/ImGuiBuild.cpp

    #Events
    src/Events/Event.h
    src/Events/ApplicationEvent.h
    src/Events/KeyEvent.h
    src/Events/MouseEvent.h

    src/Events/KeyCodes.h
    src/Events/MouseCodes.h

    # Utils
    src/Utils/PlatformUtils.cpp
    src/Utils/PlatformUtils.h

    # Math
    src/Math/Math.cpp
    src/Math/Math.h

    # Renderer
    src/Renderer/Renderer.cpp
    src/Renderer/Renderer.h

    src/Renderer/Renderer3D.cpp
    src/Renderer/Renderer3D.h

    # ------------------------------------------
    src/Renderer/Camera.h

    src/Renderer/EditorCamera.cpp
    src/Renderer/EditorCamera.h
    
    src/Renderer/OrthographicCamera.cpp
    src/Renderer/OrthographicCamera.h

    src/Renderer/OrthographicCameraController.cpp
    src/Renderer/OrthographicCameraController.h

    src/Renderer/GraphicsContext.cpp
    src/Renderer/GraphicsContext.h

    src/Renderer/UniformBuffer.cpp
    src/Renderer/UniformBuffer.h

    # ------------------------------------------

    src/Renderer/Shader.cpp
    src/Renderer/Shader.h

    src/Renderer/Buffer.cpp
    src/Renderer/Buffer.h
    
    src/Renderer/VertexArray.cpp
    src/Renderer/VertexArray.h

    src/Renderer/Texture.cpp
    src/Renderer/Texture.h


    src/Renderer/Framebuffer.cpp
    src/Renderer/Framebuffer.h

    src/Renderer/Light.cpp
    src/Renderer/Light.h

    src/Renderer/Mesh.cpp
    src/Renderer/Mesh.h
    src/Renderer/Model.cpp
    src/Renderer/Model.h

    src/Renderer/Material.cpp
    src/Renderer/Material.h

    src/Renderer/Sphere.cpp
    src/Renderer/Sphere.h

    src/Renderer/HDRSkybox.cpp
    src/Renderer/HDRSkybox.h

    # Manager
    # ------------------------------------------

    src/Renderer/Manager/TextureManager.cpp
    src/Renderer/Manager/TextureManager.h

    src/Renderer/Manager/MaterialManager.cpp
    src/Renderer/Manager/MaterialManager.h
    
    src/Renderer/Manager/ModelManager.cpp
    src/Renderer/Manager/ModelManager.h

    src/Renderer/Manager/MeshManager.cpp
    src/Renderer/Manager/MeshManager.h

    # ------------------------------------------

    src/Scene/Scene.cpp
    src/Scene/Scene.h

    src/Scene/Components.h
    src/Scene/Entity.cpp
    src/Scene/Entity.h
    src/Scene/SceneCamera.cpp
    src/Scene/SceneCamera.h

    src/Scene/SceneSerializer.cpp
    src/Scene/SceneSerializer.h

    # ------------------------------------------

    src/Panels/SceneHierarchyPanel.cpp
    src/Panels/SceneHierarchyPanel.h

    src/Panels/ProjectBrowserPanel.cpp
    src/Panels/ProjectBrowserPanel.h

    # ------------------------------------------

)

# ================    Add     ================
# Add spdlog
set(SPDLOG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/spdlog)
add_subdirectory(${SPDLOG_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/vendor/spdlog)

# Add glad
add_library(glad STATIC ${PROJECT_SOURCE_DIR}/vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC ${PROJECT_SOURCE_DIR}/vendor/glad/include)

# Add glfw
set(GLFW_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/glfw)
add_subdirectory(${GLFW_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/vendor/glfw)

# Add glm
set(GLM_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/glm)
add_subdirectory(${GLM_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/vendor/glm)

# Add stb_image
add_library(stb_image STATIC ${PROJECT_SOURCE_DIR}/vendor/stb_image/stb_image.cpp)
target_include_directories(stb_image PUBLIC ${PROJECT_SOURCE_DIR}/vendor/stb_image)

# Add Imgui
add_library(imgui STATIC
    ${PROJECT_SOURCE_DIR}/vendor/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/vendor/imgui/examples/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/vendor/imgui/examples/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${PROJECT_SOURCE_DIR}/vendor/glfw/include
    ${PROJECT_SOURCE_DIR}/vendor/glad/include
    
    ${PROJECT_SOURCE_DIR}/vendor/imgui
    ${PROJECT_SOURCE_DIR}/vendor/imgui/examples
)

# ImGuizmo
add_library(ImGuizmo STATIC ${PROJECT_SOURCE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo PUBLIC 
    ${PROJECT_SOURCE_DIR}/vendor/imgui
    ${PROJECT_SOURCE_DIR}/vendor/ImGuizmo
)

# Add Entt
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/vendor/entt/src)

# Add yam-cpp
set(YAML_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/yaml-cpp)
add_subdirectory(${YAML_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/vendor/yaml-cpp)

# ====================== Add ======================
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

target_link_libraries(${PROJECT_NAME} PRIVATE glad)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

target_link_libraries(${PROJECT_NAME} PRIVATE glm)

target_link_libraries(${PROJECT_NAME} PRIVATE stb_image)

target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE ImGuizmo)

target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)

# Copy Assets in build
file(COPY "${CMAKE_SOURCE_DIR}/Assets" DESTINATION "${CMAKE_BINARY_DIR}")
# assimp dll
file(COPY "${CMAKE_SOURCE_DIR}/vendor/Assimp/bin/" DESTINATION "${CMAKE_BINARY_DIR}")