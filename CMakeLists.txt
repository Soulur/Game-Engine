cmake_minimum_required(VERSION 3.28) # FetchContent is available in 3.11+
project(Game-Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
find_package(OpenGL REQUIRED)

# FetchContent
include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        master
      CMAKE_ARGS
    -DCMAKE_CXX_FLAGS="-Wno-error"
    -DCMAKE_CXX_FLAGS="-Wno-error=unused-variable"
    -DASSIMP_BUILD_NO_WERROR=ON
)

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG        v3.15.0
)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.2-docking
)

FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG        master
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.10.0
)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        master
    CMAKE_ARGS
    -DCMAKE_CXX_FLAGS="-Wno-error"
    -DCMAKE_CXX_FLAGS="-Wno-error=unused-variable"
    -DASSIMP_BUILD_NO_WERROR=ON
)

FetchContent_MakeAvailable(assimp entt glfw glm imgui imguizmo spdlog yaml-cpp)

# Add glad
add_library(glad STATIC ${PROJECT_SOURCE_DIR}/vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC ${PROJECT_SOURCE_DIR}/vendor/glad/include)

# Add stb_image
add_library(stb_image STATIC ${PROJECT_SOURCE_DIR}/vendor/stb_image/stb_image.cpp)
target_include_directories(stb_image PUBLIC ${PROJECT_SOURCE_DIR}/vendor/stb_image)

# Add ImGui 库
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glad glfw)

# Add ImGuizmo 库
add_library(ImGuizmo STATIC ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)
target_include_directories(ImGuizmo PUBLIC 
    ${imguizmo_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)
target_link_libraries(ImGuizmo PUBLIC imgui) # 链接到我们自己创建的 ImGui 库

# Add Entt
add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${entt_SOURCE_DIR}/src)

# =============================================

# 在顶层 CMakeLists.txt 中设置全局包含路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
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

    src/Renderer/Manager/HdrManager.cpp
    src/Renderer/Manager/HdrManager.h

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
# =============================================

target_link_libraries(${PROJECT_NAME} 
  PUBLIC
    OpenGL::GL

    glad
    stb_image

    imgui
    ImGuizmo
    entt
    
    
    # FetchContent_MakeAvailable
    glm
    assimp::assimp
    glfw
    spdlog::spdlog
    yaml-cpp
)

# Copy Assets in build
file(COPY "${CMAKE_SOURCE_DIR}/Assets" DESTINATION "${CMAKE_BINARY_DIR}")

# 自动将所有依赖的动态库复制到可执行文件所在的目录
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:glad>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:glfw>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:assimp>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:spdlog>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:yaml-cpp>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:ImGuizmo>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)