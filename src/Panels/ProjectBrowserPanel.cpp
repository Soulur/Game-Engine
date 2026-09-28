#include "ProjectBrowserPanel.h"

#include <algorithm>

#include <imgui.h>
#include <imgui_internal.h>

namespace Mc {

	// Once we have projects change this
	extern const std::filesystem::path g_AssetPath = "Assets";

	ProjectBrowserPanel::ProjectBrowserPanel()
		: m_CurrentDirectory(g_AssetPath)
	{
		m_FolderIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/folder.png");

		m_FolderMaxIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/folderMax.png");
		m_FolderZipMaxIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/folder-zipMax.png");
		m_CubeOutlineIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/cube-outline.png");
		m_MaterialIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/sphere.png");
		m_ShaderIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/auto-fix.png");
		m_TextureIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/texture-box.png");
		m_SceneIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/floor-plan.png");
		m_AudioIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/music-note.png");
		m_DocumentIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/file-document-outline.png");
		m_OtherIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/file-question.png");
		m_CodeIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/code-block-braces.png");

		m_ArrowLeftIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/arrow-left.png");
		m_ArrowLeftSelectIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/arrow-left-Select.png");
		m_ArrowRightIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/arrow-right.png");
		m_ArrowRightSelectIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/arrow-right-Select.png");

		m_NoSearchIcon = Texture2D::Create("Resources/Icons/DirectoryIcon/magnify-close.png");
	}

	// 获取文件图标类型
	FileIconType GetFileIconType(const std::filesystem::path &path)
	{
		if (std::filesystem::is_directory(path))
			return FileIconType::Folder;

		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		// 1. 模型/几何体 (Meshes)
		if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".stl" || ext == ".dae")
			return FileIconType::Mesh;

		// 2. 贴图/图片 (Textures/Images)
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds" || ext == ".hdr" || ext == ".bmp")
			return FileIconType::Image;

		// 3. 着色器 (Shaders)
		if (ext == ".glsl" || ext == ".hlsl" || ext == ".shader" || ext == ".vert" || ext == ".frag" || ext == ".comp")
			return FileIconType::Shader;

		// 4. 材质 (Materials)
		if (ext == ".mat" || ext == ".mtl" || ext == ".material")
			return FileIconType::Material;

		// 5. 引擎对象 (Scenes/Prefabs)
		if (ext == ".scene")
			return FileIconType::Scene;

		// 6. 音频 (Audio)
		if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac")
			return FileIconType::Audio;

		// 7. 脚本/代码 (Code)
		if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cs" || ext == ".lua" || ext == ".py" || ext == ".js")
			return FileIconType::Code;

		// 8. 压缩包 (Zip)
		if (ext == ".zip" || ext == ".7z" || ext == ".rar" || ext == ".tar" || ext == ".gz")
			return FileIconType::Zip;

		return FileIconType::Other;
	}

	void ProjectBrowserPanel::RenderTopBar()
    {
		float ts = ImGui::GetIO().DeltaTime;

		ImGui::BeginChild("TopBar", ImVec2(0, 35), false, ImGuiWindowFlags_NoScrollbar);
		{
			float searchBarHeight = 32.0f;
			{
				bool canGoBack = !m_BackStack.empty();

				if (!canGoBack)
					ImGui::BeginDisabled();
				ImTextureID icon = !canGoBack ? (ImTextureID)m_ArrowLeftIcon->GetRendererID() : (ImTextureID)m_ArrowLeftSelectIcon->GetRendererID();
				if (ImGui::ImageButton("##canGoBack", icon, ImVec2(searchBarHeight, searchBarHeight), ImVec2(0, 0), ImVec2(1, 1)))
				{
					m_ForwardStack.push_back(g_SelectedItem);
					g_SelectedItem = m_BackStack.back();
					m_BackStack.pop_back();
				}
				if (!canGoBack)
					ImGui::EndDisabled();
			}

			ImGui::SameLine();

			{
				bool canGoForward = !m_ForwardStack.empty();

				if (!canGoForward)
					ImGui::BeginDisabled();
				ImTextureID icon = !canGoForward ? (ImTextureID)m_ArrowRightIcon->GetRendererID() : (ImTextureID)m_ArrowRightSelectIcon->GetRendererID();
				if (ImGui::ImageButton("##canGoForward", icon, ImVec2(searchBarHeight, searchBarHeight), ImVec2(0, 0), ImVec2(1, 1)))
				{
					m_BackStack.push_back(g_SelectedItem);
					g_SelectedItem = m_ForwardStack.back();
					m_ForwardStack.pop_back();
				}
				if (!canGoForward)
					ImGui::EndDisabled();
			}

			ImGui::SameLine();
			
			float fontSize = ImGui::GetFontSize();
			float paddingY = (searchBarHeight - fontSize) * 0.5f;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, paddingY));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

			ImGui::SetNextItemWidth(250);
			if (ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, sizeof(m_SearchBuffer)))
			{
			}
			ImGui::PopStyleVar(3);

			ImGui::SameLine();

			std::filesystem::path relativePath = std::filesystem::relative(g_SelectedItem, g_AssetPath);
			std::string testPath = "Assets";
			
			for (const auto &part : relativePath)
			{
				if (part == "." || part.empty()) continue;
				testPath += "/" + part.string();
			}
			ImGui::TextDisabled(testPath.c_str());
		}
		ImGui::EndChild();
	}

    void ProjectBrowserPanel::RenderContentGrid()
    {
		float padding = 16.0f;
		float thumbnailSize = 100.0f; // 建议大小，过大会导致图标原图拉伸模糊
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		bool showNoResults = false;
		// 使用表格布局网格
		if (ImGui::BeginTable("ContentGrid", columnCount, ImGuiTableFlags_NoSavedSettings))
		{
			if (std::filesystem::exists(g_SelectedItem) && std::filesystem::is_directory(g_SelectedItem))
			{
				std::string searchQuery = m_SearchBuffer;
				std::vector<std::filesystem::directory_entry> entries;
				for (const auto &entry : std::filesystem::directory_iterator(g_SelectedItem))
				{
					std::string filename = entry.path().filename().string();
					std::string filenameLower = filename;
					std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);

					if (!searchQuery.empty())
						if (filenameLower.find(searchQuery) == std::string::npos)
							continue;

					entries.push_back(entry);
				}

				if (entries.empty() && !searchQuery.empty())
				{
					showNoResults = true;
				}

				std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
					if (a.is_directory() != b.is_directory())
						return a.is_directory() > b.is_directory(); 
					return a.path().filename().string() < b.path().filename().string(); 
				});

				for (auto &entry : entries)
				{
					const auto &path = entry.path();
					std::string filename = path.filename().string();
					bool is_directory = entry.is_directory();

					ImGui::TableNextColumn();

					ImGui::PushID(filename.c_str());
					ImGui::BeginGroup();

					{
						ImTextureID iconTex;
						FileIconType type = GetFileIconType(path);
						switch (type)
						{
							case FileIconType::Folder:		iconTex = m_FolderMaxIcon->GetRendererID(); break;
							case FileIconType::Mesh:		iconTex = m_CubeOutlineIcon->GetRendererID(); break;
							case FileIconType::Material:	iconTex = m_MaterialIcon->GetRendererID(); break;
							case FileIconType::Shader:		iconTex = m_ShaderIcon->GetRendererID(); break;
							case FileIconType::Scene:		iconTex = m_SceneIcon->GetRendererID(); break;
							case FileIconType::Image:		iconTex = m_TextureIcon->GetRendererID(); break;
							case FileIconType::Audio:		iconTex = m_AudioIcon->GetRendererID(); break;
							case FileIconType::Zip:			iconTex = m_FolderZipMaxIcon->GetRendererID(); break;
							case FileIconType::Document:	iconTex = m_DocumentIcon->GetRendererID();break;
							case FileIconType::Code:		iconTex = m_CodeIcon->GetRendererID();break;
							default:						iconTex = m_OtherIcon->GetRendererID();break;
						}

						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						if (ImGui::ImageButton("##thumb", iconTex, ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 1), ImVec2(1, 0)))
						{
						}
						ImGui::PopStyleColor();
					}

					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						if (is_directory)
						{
							m_BackStack.push_back(g_SelectedItem);
							g_SelectedItem = path;
							m_ForwardStack.clear();
						}
					}

					{
						float textWidth = thumbnailSize;
						ImVec2 pos = ImGui::GetCursorScreenPos();
						ImVec2 textSize = ImGui::CalcTextSize(filename.c_str());

						// 如果文字宽度超过了图标宽度
						if (textSize.x > textWidth)
						{
							ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(),
													  pos,
													  ImVec2(pos.x + textWidth, pos.y + ImGui::GetTextLineHeight()),
													  pos.x + textWidth,
													  filename.c_str(),
													  nullptr,
													  &textSize);
							ImGui::Dummy(ImVec2(textWidth, ImGui::GetTextLineHeight()));
						}
						else
						{
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (textWidth - textSize.x) * 0.5f);
							ImGui::TextUnformatted(filename.c_str());
						}

						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("%s", filename.c_str());
					}


					ImGui::EndGroup();
					ImGui::PopID();
				}
			}
			ImGui::EndTable();
		}
		if (showNoResults)
		{
			ImVec2 windowSize = ImGui::GetContentRegionAvail();
			ImVec2 cursorStart = ImGui::GetCursorPos();

			float iconDisplaySize = 64.0f;
			float textHeight = ImGui::GetTextLineHeight();
			float totalHeight = iconDisplaySize + textHeight + 10.0f;

			float centerY = (windowSize.y - totalHeight) * 0.5f;
			if (centerY > 0)
				ImGui::SetCursorPosY(cursorStart.y + centerY);

			float centerX = (windowSize.x - iconDisplaySize) * 0.5f;
			ImGui::SetCursorPosX(cursorStart.x + centerX);

			ImGui::Image(m_NoSearchIcon->GetRendererID(), ImVec2(iconDisplaySize, iconDisplaySize), ImVec2(0, 1), ImVec2(1, 0));
		}
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

			std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b)
					  {
            if (a.is_directory() != b.is_directory())
                return a.is_directory() > b.is_directory();
            return a.path().filename().string() < b.path().filename().string(); });

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

			for (const auto &entry : entries)
			{
				bool is_folder = entry.is_directory();
				if (!is_folder) break;;

				const auto &filename = entry.path().filename().string();

				ImGui::PushID(filename.c_str());

				ImGuiID nodeID = ImGui::GetID("##node");

				// 1. 设置树节点标志
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
				if (g_SelectedItem == entry.path())
					flags |= ImGuiTreeNodeFlags_Selected;
				if (!is_folder) flags |= ImGuiTreeNodeFlags_Leaf;

				bool open = ImGui::TreeNodeEx("##node", flags, "");

				if (ImGui::IsItemHovered())
				{

					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						ImGuiStorage *storage = ImGui::GetStateStorage();
						bool *p_open = storage->GetBoolRef(nodeID);
						*p_open = !*p_open;
					}
					else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
					{
						m_BackStack.push_back(g_SelectedItem);
						g_SelectedItem = entry.path();
						m_ForwardStack.clear();
					}
				}

				ImGui::SameLine();
				if (g_ShowFileIcons && is_folder)
				{
					float iconSize = ImGui::GetTextLineHeight();
					ImGui::Image(m_FolderIcon->GetRendererID(), ImVec2(iconSize, iconSize), ImVec2(0, 1), ImVec2(1, 0));
					ImGui::SameLine();
				}

				ImGui::TextUnformatted(filename.c_str());

				if (open)
				{
					if (is_folder) DisplayFileTree(entry.path());
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::PopStyleVar(2);
		}
		catch (const std::filesystem::filesystem_error &e)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error accessing %s: %s", path.string().c_str(), e.what());
		}
	}

	void ProjectBrowserPanel::OnImGuiRender()
	{		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::Begin("Content Browser");
		ImGui::PopStyleColor();

		// 1. 创建无边框表格作为主容器
		static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchSame;
		if (ImGui::BeginTable("BrowserMainLayout", 2, tableFlags))
		{
			ImGui::TableSetupColumn("TreeColumn", ImGuiTableColumnFlags_WidthFixed, 220.0f);
			ImGui::TableSetupColumn("GridColumn", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

			if (ImGui::BeginChild("##TreeRegion", ImVec2(0, 0), false))
			{
				ImGui::Spacing();
				DisplayFileTree(g_AssetPath);
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::TableSetColumnIndex(1);

			if (ImGui::BeginChild("##GridRegion", ImVec2(0, 0), false))
			{
				RenderTopBar();		// 渲染导航栏
				ImGui::Separator(); // 淡淡的分隔线
				ImGui::Spacing();
				RenderContentGrid(); // 渲染网格图标
			}
			ImGui::EndChild();
			ImGui::EndTable();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

}