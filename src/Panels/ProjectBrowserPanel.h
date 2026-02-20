#pragma once

#include "src/Renderer/Texture.h"

#include <filesystem>

namespace Mc {

	// 文件类型图标
	enum class FileIconType
	{
		Folder, // 文件夹
		Code,
		Image,
		Document,
		Other
	};

	class ProjectBrowserPanel
	{
	public:
		ProjectBrowserPanel();

		void DrawIcon(FileIconType type);
		void RenderTopBar();
		void RenderContentGrid();
		void DisplayFileTree(const std::filesystem::path &path);
		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;
		std::string g_SearchFilter;
		std::filesystem::path g_SelectedItem;
		bool g_ShowFileIcons = true;

		Ref<Texture2D> m_Folder;
	};

}