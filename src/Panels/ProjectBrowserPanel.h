#pragma once

#include "src/Renderer/Texture.h"

#include <filesystem>

namespace Mc {

	// 文件类型图标
	enum class FileIconType
	{
		Folder,
		Mesh,
		Material,
		Shader,
		Scene,
		Image,
		Audio,
		Code,
		Zip,
		Document,
		Other
	};

	class ProjectBrowserPanel
	{
	public:
		ProjectBrowserPanel();

		void RenderTopBar();
		void RenderContentGrid();
		void DisplayFileTree(const std::filesystem::path &path);
		void OnImGuiRender();
	private:
		std::filesystem::path m_CurrentDirectory;
		std::string g_SearchFilter;
		std::filesystem::path g_SelectedItem;
		bool g_ShowFileIcons = true;

		std::vector<std::filesystem::path> m_BackStack;
		std::vector<std::filesystem::path> m_ForwardStack;
		char m_SearchBuffer[256] = "";

		Ref<Texture2D> m_FolderIcon,
			m_FolderMaxIcon, m_FolderZipMaxIcon,
			m_CubeOutlineIcon, m_MaterialIcon, m_ShaderIcon, m_TextureIcon, m_SceneIcon,
			m_DocumentIcon, m_AudioIcon,
			m_CodeIcon,
			m_OtherIcon,
			m_ArrowLeftIcon, m_ArrowLeftSelectIcon,
			m_ArrowRightIcon, m_ArrowRightSelectIcon,
			m_NoSearchIcon
			;
	};

}