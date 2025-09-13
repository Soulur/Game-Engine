# Game-Engine

这是一个使用 C++ 和 OpenGL 构建的轻量级游戏引擎。

## 编译项目
本项目使用 **CMake** 构建。请确保你的系统已安装 **CMake 3.11** 或更高版本，以及一个 C++ 编译器（例如 GCC, Clang 或 MSVC）。

1.  **生成构建文件**

    在项目根目录中创建一个 `build` 文件夹并进入。

    ```sh
    mkdir build
    cd build
    ```

2.  **运行 CMake**

    在 `build` 目录中运行 `cmake` 命令。

    * **在 Windows 上**:
        
        ```sh
        cmake -G "MinGW Makefiles" ..
        ```
        
        > 注意: `-G "MinGW Makefiles"` 参数指定了 MinGW 作为构建系统。
    
    * **在 Linux / macOS 上**:
        
        ```sh
        cmake ..
        ```
        
3.  **编译项目**

    运行以下命令即可编译整个项目。

    ```sh
    cmake --build .
    ```

## 项目依赖

本项目使用了以下第三方库。

1.  **系统依赖**

    本项目通过 `find_package(OpenGL REQUIRED)` 链接 **OpenGL**。这意味着你需要在系统上安装 OpenGL 的开发库。

    * **在 Linux 上**：OpenGL 的实现（如 Mesa 或 NVIDIA 驱动）通常与开发库分开提供。你需要通过系统的包管理器安装相应的开发包。
        * **对于 Ubuntu/Debian**：
            ```sh
            sudo apt-get install libgl1-mesa-dev
            ```
        * **对于 Fedora/CentOS**：
            ```sh
            sudo dnf install mesa-libGL-devel
            ```
        * **对于 Arch Linux**：
            ```sh
            sudo pacman -S mesa
            ```

    * **在 Windows 上**：通常，OpenGL 的开发文件会随显卡驱动一起安装。如果遇到问题，请确保你的显卡驱动是最新的。你也可以考虑安装像 Mesa3D 这样的软件实现来测试。


### 通过 CMake 自动下载的依赖

这些库都已通过 CMake 的 `FetchContent` 模块集成在项目中，你无需手动下载或安装。当你运行 `cmake` 命令时，它们会自动下载并编译。

* **[Assimp](https://github.com/assimp/assimp)**：用于导入各种 3D 模型格式。
* **[Entt](https://github.com/skypjack/entt)**：用于构建数据驱动的游戏和应用程序。
* **[GLFW](https://github.com/GLFW/GLFW)**：用于创建窗口、上下文和处理用户输入。
* **[GLM](https://github.com/g-truc/glm)**：用于进行数学运算，如向量和矩阵。
* **[ImGui](https://github.com/ocornut/imgui)**：用于创建图形用户界面。
* **[ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)**：用于在 3D 场景中对物体进行平移、旋转和缩放操作。
* **[Spdlog](https://github.com/gabime/spdlog)**：一个快速、易于使用的日志库。
* **[yaml-cpp](https://github.com/jbeder/yaml-cpp)**：用于处理 YAML 配置文件。

### 本地依赖

以下库已包含在项目的 `vendor/` 文件夹中，作为本地依赖，无需额外下载或配置。

* **[glad](https://github.com/Dav1dde/glad)**：用于加载 OpenGL 函数指针的库。
* **[stb_image](https://github.com/nothings/stb)**：一个用于加载各种图像文件格式的单文件头库。
