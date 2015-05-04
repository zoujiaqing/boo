#ifndef IGRAPHICSCONTEXT_HPP
#define IGRAPHICSCONTEXT_HPP

namespace boo
{

class IGraphicsContext
{
public:
    
    enum EGraphicsAPI
    {
        API_NONE       = 0,
        API_OPENGL_3_3 = 1,
        API_OPENGL_4_2 = 2,
        API_OPENGLES_3 = 3,
        API_VULKAN     = 4,
        API_D3D11      = 5,
        API_METAL      = 6
    };
    
    enum EPixelFormat
    {
        PF_NONE        = 0,
        PF_RGBA8       = 1, /* Default */
        PF_RGBA8_Z24   = 2,
        PF_RGBAF32     = 3,
        PF_RGBAF32_Z24 = 4
    };
    
    virtual ~IGraphicsContext() {}
    
    virtual EGraphicsAPI getAPI() const=0;
    virtual EPixelFormat getPixelFormat() const=0;
    virtual void setPixelFormat(EPixelFormat pf)=0;
    virtual void setPlatformWindowHandle(void* handle)=0;
    virtual void initializeContext()=0;
    virtual IGraphicsContext* makeShareContext() const=0;
    virtual void makeCurrent()=0;
    virtual void clearCurrent()=0;

    /* Note: *all* contexts are double-buffered with
     * v-sync interval; please call this */
    virtual void swapBuffer()=0;
    
};
    
IGraphicsContext* IGraphicsContextNew(IGraphicsContext::EGraphicsAPI api);
    
}

#endif // IGRAPHICSCONTEXT_HPP
