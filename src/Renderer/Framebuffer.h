#pragma once

#include "src/Core/Base.h"

#include <initializer_list>
#include <vector>

namespace Mc {

    enum class FramebufferTextureFormat
    {
        None = 0,

        // Color
        RGBA8,
        RED_INTEGER,
        // DirShadow Depth
        DIRSHADOW,
        // TODO: PointShadow Depth
        // Depth /stencils
        DEPTH24STENCIL8,

        // Defaults
        // Depth = DEPTH24STENCIL8,
    };

    struct FramebufferTextureSpecification
    {
        FramebufferTextureSpecification() = default;
        FramebufferTextureSpecification(FramebufferTextureFormat format)
            : TextureFormat(format) {}

        FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
        // TODO: filtering/wrap
    };

    struct FramebufferAttachmentSpecification
    {
        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
            : Attachments(attachments) {}

        std::vector<FramebufferTextureSpecification> Attachments;
    };

    struct FramebufferSpecification
    {
        uint32_t Width = 0, Height = 0;
        FramebufferAttachmentSpecification Attachments;
        uint32_t Samples = 1;

        bool SwapChainTarget = false;
    };

    class Framebuffer
    {
    public:
        Framebuffer(const FramebufferSpecification &spec);
        ~Framebuffer();

        void Invalidate();

        void Bind();
        void UnBind();

        void Resize(uint32_t width, uint32_t height);
        int ReadPixel(uint32_t attachmentIndex, int x, int y);

        void ClearAttachment(uint32_t attachmentIndex, int value);

        uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const;
        uint32_t GetDepthAttachmentRendererID() const { return m_DepthAttachment; };

        // 启动帧缓冲中的深度纹理
        void BindDepthAttachment(int slot = 0);

        const FramebufferSpecification &GetSpecification() const;

        static Ref<Framebuffer> Create(const FramebufferSpecification &spec);
    private:
        uint32_t m_RendererID = 0;
        
        FramebufferSpecification m_Specification;

        std::vector<uint32_t> m_ColorAttachments;
        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        
        uint32_t m_DepthAttachment = 0;
        FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;
    };
}