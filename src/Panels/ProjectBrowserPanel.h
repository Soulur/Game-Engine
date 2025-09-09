#pragma once

#include "src/Renderer/Texture.h"

#include <filesystem>

namespace Mc {

	class ProjectBrowserPanel
	{
	public:
		ProjectBrowserPanel();

		void DisplayFileTree(const std::filesystem::path &path);
		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;
		std::string g_SearchFilter;
		std::filesystem::path g_SelectedItem;
		bool g_ShowFileIcons = true;
	};

}