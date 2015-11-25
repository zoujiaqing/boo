#ifndef GDEV_GLES3_HPP
#define GDEV_GLES3_HPP

#include "IGraphicsDataFactory.hpp"
#include "IGraphicsCommandQueue.hpp"
#include "boo/IGraphicsContext.hpp"
#include <vector>
#include <unordered_set>

namespace boo
{

class GLDataFactory : public IGraphicsDataFactory
{
    IGraphicsContext* m_parent;
    IGraphicsData* m_deferredData = nullptr;
    std::unordered_set<IGraphicsData*> m_committedData;
    std::vector<int> m_texUnis;
public:
    GLDataFactory(IGraphicsContext* parent);
    ~GLDataFactory() {}

    Platform platform() const {return Platform::OGL;}
    const SystemChar* platformName() const {return _S("OGL");}

    IGraphicsBufferS* newStaticBuffer(BufferUse use, const void* data, size_t stride, size_t count);
    IGraphicsBufferS* newStaticBuffer(BufferUse use, std::unique_ptr<uint8_t[]>&& data, size_t stride, size_t count);
    IGraphicsBufferD* newDynamicBuffer(BufferUse use, size_t stride, size_t count);

    ITextureS* newStaticTexture(size_t width, size_t height, size_t mips, TextureFormat fmt,
                                const void* data, size_t sz);
    ITextureS* newStaticTexture(size_t width, size_t height, size_t mips, TextureFormat fmt,
                                std::unique_ptr<uint8_t[]>&& data, size_t sz);
    ITextureS* newStaticArrayTexture(size_t width, size_t height, size_t layers, TextureFormat fmt,
                                    const void* data, size_t sz);
    ITextureD* newDynamicTexture(size_t width, size_t height, TextureFormat fmt);
    ITextureR* newRenderTexture(size_t width, size_t height, size_t samples);

    bool bindingNeedsVertexFormat() const {return true;}
    IVertexFormat* newVertexFormat(size_t elementCount, const VertexElementDescriptor* elements);

    IShaderPipeline* newShaderPipeline(const char* vertSource, const char* fragSource,
                                       size_t texCount, const char* texArrayName,
                                       size_t uniformBlockCount, const char** uniformBlockNames,
                                       BlendFactor srcFac, BlendFactor dstFac,
                                       bool depthTest, bool depthWrite, bool backfaceCulling);

    IShaderDataBinding*
    newShaderDataBinding(IShaderPipeline* pipeline,
                         IVertexFormat* vtxFormat,
                         IGraphicsBuffer* vbo, IGraphicsBuffer* instVbo, IGraphicsBuffer* ibo,
                         size_t ubufCount, IGraphicsBuffer** ubufs,
                         size_t texCount, ITexture** texs);

    void reset();
    IGraphicsData* commit();
    void destroyData(IGraphicsData*);
    void destroyAllData();
};

}

#endif // GDEV_GLES3_HPP
