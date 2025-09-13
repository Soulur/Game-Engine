#include "ProjectBrowserPanel.h"

#include <algorithm>

#include <imgui.h>

namespace Mc {

	// Once we have projects change this
	extern const std::filesystem::path g_AssetPath = "Assets";

	ProjectBrowserPanel::ProjectBrowserPanel()
		: m_CurrentDirectory(g_AssetPath)
	{
	}


	// 文件类型图标
	enum class FileIconType
	{
		Folder,	// 文件夹
		Code,
		Image,
		Document,
		Other
	};

	// 获取文件图标类型
	FileIconType GetFileIconType(const std::filesystem::path &path)
	{
		if (std::filesystem::is_directory(path))
			return FileIconType::Folder;

		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cs" || ext == ".js" || ext == ".ts")
			return FileIconType::Code;
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif")
			return FileIconType::Image;
		if (ext == ".txt" || ext == ".md" || ext == ".pdf" || ext == ".doc" || ext == ".docx")
			return FileIconType::Document;

		return FileIconType::Other;
	}

	// 绘制图标 (实际项目中应该使用纹理图标)
	void DrawIcon(FileIconType type)
	{
		const char *icon = "?";
		ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Text];

		switch (type)
		{
			case FileIconType::Folder:
				icon = "[D]";
				color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
				break;
			case FileIconType::Code:
				icon = "</>";
				color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
				break;
			case FileIconType::Image:
				icon = "[I]";
				color = ImVec4(0.8f, 0.4f, 0.8f, 1.0f);
				break;
			case FileIconType::Document:
				icon = "[T]";
				color = ImVec4(0.6f, 0.8f, 0.2f, 1.0f);
				break;
			default:
				icon = "[F]";
				break;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::Text(icon);
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}

	// 递归显示文件树
	void ProjectBrowserPanel::DisplayFileTree(const std::filesystem::path &path)
	{
		try
		{
			// 1. 收集所有目录和文件
			std::vector<std::filesystem::directory_entry> entries;
			for (const auto &entry : std::filesystem::directory_iterator(path))
			{
				entries.push_back(entry);
			}

			// 2. 排序 - 目录在前，文件在后
			std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b)
					  {
            if (a.is_directory() != b.is_directory())
                return a.is_directory() > b.is_directory();
            return a.path().filename().string() < b.path().filename().string(); });

			// 3. 显示条目
			for (const auto &entry : entries)
			{
				const auto &filename = entry.path().filename().string();
				bool is_folder = entry.is_directory();

				// 跳过不符合搜索条件的项目
				if (!g_SearchFilter.empty() && filename.find(g_SearchFilter) == std::string::npos)
					continue;

				ImGui::PushID(filename.c_str());

				// 获取行起始位置
				ImVec2 line_start = ImGui::GetCursorPos();

				// 绘制图标
				if (g_ShowFileIcons)
				{
					DrawIcon(is_folder ? FileIconType::Folder : GetFileIconType(entry.path()));
					ImGui::SameLine();
				}

				// 创建透明按钮覆盖整行
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float line_height = ImGui::GetTextLineHeightWithSpacing();
				ImGui::InvisibleButton("##row_btn", ImVec2(avail.x, line_height));

				// 处理文件夹/文件交互 - 修改后的点击处理
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
				{
					g_SelectedItem = entry.path();
					if (is_folder)
					{
						// 手动触发展开/折叠
						bool *p_open = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID(filename.c_str()));
						*p_open = !*p_open; // 切换展开状态
					}
				}

				// 绘制文件名和树节点
				ImGui::SetCursorPos(line_start);
				if (g_ShowFileIcons)
				{
					ImGui::SetCursorPosX(line_start.x + ImGui::GetTextLineHeight());
				}

				if (is_folder)
				{
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
					if (g_SelectedItem == entry.path())
						flags |= ImGuiTreeNodeFlags_Selected;

					// 获取当前展开状态
					bool *p_open = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID(filename.c_str()));
					bool is_open = ImGui::TreeNodeEx(filename.c_str(), flags, "%s", filename.c_str());

					// 同步状态
					if (is_open != *p_open) *p_open = is_open;

					if (is_open)
					{
						DisplayFileTree(entry.path());
						ImGui::TreePop();
					}
				}
				else
				{
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth;
					if (g_SelectedItem == entry.path())
						flags |= ImGuiTreeNodeFlags_Selected;

					// 处理拖拽
					if (ImGui::BeginDragDropSource())
					{
						auto relativePath = std::filesystem::relative(path, g_AssetPath);
						const wchar_t *itemPath = (relativePath / filename).wstring().c_str();
						ImGui::SetDragDropPayload("FOLDER_PANEL", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
						ImGui::EndDragDropSource();
					}

					ImGui::TreeNodeEx(filename.c_str(), flags);
				}

				ImGui::PopID();
			}
		}
		catch (const std::filesystem::filesystem_error &e)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error accessing %s: %s", path.string().c_str(), e.what());
		}
	}

	void ProjectBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_MenuBar);

		// ------------------------
		// 菜单栏：导航按钮
		// ------------------------
		static std::string currentFilter;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Show Icons", nullptr, &g_ShowFileIcons);
				ImGui::EndMenu();
			}

			static char searchBuffer[256] = "";	

			// 搜索框
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
			if (ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer)))
			{
				currentFilter = searchBuffer;
			}

			ImGui::EndMenuBar();
		}

		// 文件树
		if (ImGui::BeginChild("##FileTree", ImVec2(0, 0), true))
			DisplayFileTree(g_AssetPath);
		
		ImGui::EndChild();

		ImGui::End();

	}

}