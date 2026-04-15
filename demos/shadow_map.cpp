// Shadow Map Demo – follows hello_triangle patterns exactly
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include "Cacao.hpp"

using namespace Cacao;

struct Vertex { float pos[3]; float normal[3]; float color[3]; };

struct SceneParams {
    float cameraVP[16];
    float lightVP[16];
    float lightPos[3]; float _pad0;
    float cameraPos[3]; float _pad1;
};

static float g_yaw = 0.0f, g_pitch = 0.25f, g_dist = 10.0f;
static float g_cx = 0, g_cy = 0.5f, g_cz = 0;
static bool g_drag = false;
static int g_lx = 0, g_ly = 0;

static void MakeLookAt(float* o, float ex, float ey, float ez, float cx, float cy, float cz)
{
    float fx=cx-ex, fy=cy-ey, fz=cz-ez;
    float fl=sqrtf(fx*fx+fy*fy+fz*fz);
    fx/=fl; fy/=fl; fz/=fl;
    float sx=fy*0-fz*1, sy=fz*0-fx*0, sz=fx*1-fy*0;
    float sl=sqrtf(sx*sx+sy*sy+sz*sz);
    if(sl<1e-6f){sx=1;sy=0;sz=0;sl=1;}
    sx/=sl;sy/=sl;sz/=sl;
    float ux=sy*fz-sz*fy, uy=sz*fx-sx*fz, uz=sx*fy-sy*fx;
    memset(o,0,64);
    o[0]=sx;o[1]=ux;o[2]=-fx;
    o[4]=sy;o[5]=uy;o[6]=-fy;
    o[8]=sz;o[9]=uz;o[10]=-fz;
    o[12]=-(sx*ex+sy*ey+sz*ez);
    o[13]=-(ux*ex+uy*ey+uz*ez);
    o[14]=(fx*ex+fy*ey+fz*ez);
    o[15]=1;
}

static void MakePerspective(float* o, float fov, float asp, float zn, float zf)
{
    float t=tanf(fov*0.5f);
    memset(o,0,64);
    o[0]=1.0f/(asp*t);
    o[5]=-1.0f/t;
    o[10]=-zf/(zf-zn);
    o[11]=-1.0f;
    o[14]=-zf*zn/(zf-zn);
}

static void MatMul(const float* a, const float* b, float* o)
{
    float t[16];
    for(int c=0;c<4;c++) for(int r=0;r<4;r++){
        t[c*4+r]=0;
        for(int k=0;k<4;k++) t[c*4+r]+=a[k*4+r]*b[c*4+k];
    }
    memcpy(o,t,64);
}

static void BuildScene(std::vector<Vertex>& V, std::vector<uint32_t>& I)
{
    auto quad=[&](const float p[4][3], const float n[3], const float c[3]){
        uint32_t b=(uint32_t)V.size();
        for(int i=0;i<4;i++) V.push_back({{p[i][0],p[i][1],p[i][2]},{n[0],n[1],n[2]},{c[0],c[1],c[2]}});
        I.insert(I.end(),{b,b+1,b+2,b,b+2,b+3});
    };
    { float p[4][3]={{-7,-1,-7},{7,-1,-7},{7,-1,7},{-7,-1,7}}; float n[]={0,1,0},c[]={0.78f,0.76f,0.72f}; quad(p,n,c); }
    { float p[4][3]={{-4,-1,-6},{-4,-1,6},{-4,6,6},{-4,6,-6}}; float n[]={1,0,0},c[]={0.72f,0.75f,0.80f}; quad(p,n,c); }
    { float p[4][3]={{-4,-1,-5},{4,-1,-5},{4,6,-5},{-4,6,-5}}; float n[]={0,0,1},c[]={0.80f,0.77f,0.73f}; quad(p,n,c); }
    {
        float h=0.7f, cy=-0.3f; float cc[]={0.92f,0.44f,0.13f};
        {float p[4][3]={{-h,cy+h,-h},{h,cy+h,-h},{h,cy+h,h},{-h,cy+h,h}};float n[]={0,1,0};quad(p,n,cc);}
        {float p[4][3]={{-h,cy-h,h},{h,cy-h,h},{h,cy-h,-h},{-h,cy-h,-h}};float n[]={0,-1,0};quad(p,n,cc);}
        {float p[4][3]={{-h,cy-h,-h},{h,cy-h,-h},{h,cy+h,-h},{-h,cy+h,-h}};float n[]={0,0,-1};quad(p,n,cc);}
        {float p[4][3]={{h,cy-h,h},{-h,cy-h,h},{-h,cy+h,h},{h,cy+h,h}};float n[]={0,0,1};quad(p,n,cc);}
        {float p[4][3]={{-h,cy-h,h},{-h,cy-h,-h},{-h,cy+h,-h},{-h,cy+h,h}};float n[]={-1,0,0};quad(p,n,cc);}
        {float p[4][3]={{h,cy-h,-h},{h,cy-h,h},{h,cy+h,h},{h,cy+h,-h}};float n[]={1,0,0};quad(p,n,cc);}
    }
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if(m==WM_CLOSE){PostQuitMessage(0);return 0;}
    if(m==WM_KEYDOWN&&w==VK_ESCAPE){PostQuitMessage(0);return 0;}
    if(m==WM_LBUTTONDOWN){g_drag=true;g_lx=LOWORD(l);g_ly=HIWORD(l);SetCapture(h);return 0;}
    if(m==WM_LBUTTONUP){g_drag=false;ReleaseCapture();return 0;}
    if(m==WM_MOUSEMOVE&&g_drag){
        int mx=LOWORD(l),my=HIWORD(l);
        g_yaw-=(float)(mx-g_lx)*0.005f;
        g_pitch+=(float)(my-g_ly)*0.005f;
        g_pitch=std::clamp(g_pitch,-1.5f,1.5f);
        g_lx=mx;g_ly=my;return 0;
    }
    if(m==WM_MOUSEWHEEL){
        g_dist-=(float)GET_WHEEL_DELTA_WPARAM(w)/120.0f*0.5f;
        g_dist=std::clamp(g_dist,2.0f,30.0f);return 0;
    }
    return DefWindowProc(h,m,w,l);
}

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const uint32_t SM=2048;

    HINSTANCE hI=GetModuleHandle(nullptr);
    WNDCLASSEX wc={};
    wc.cbSize=sizeof(WNDCLASSEX);
    wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wc.lpfnWndProc=WndProc;
    wc.hInstance=hI;
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName="ShadowMapClass";
    RegisterClassEx(&wc);

    RECT wr={0,0,1024,768};
    AdjustWindowRect(&wr,WS_OVERLAPPEDWINDOW,FALSE);
    HWND hW=CreateWindowEx(0,"ShadowMapClass","Cacao - Shadow Map Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,
        wr.right-wr.left,wr.bottom-wr.top,nullptr,nullptr,hI,nullptr);

    BackendType backend=BackendType::DirectX12;

    InstanceCreateInfo ici;
    ici.type=backend;
    ici.applicationName="ShadowMap";
    ici.enabledFeatures={InstanceFeature::ValidationLayer,InstanceFeature::Surface};
    auto inst=Instance::Create(ici);
    auto adap=inst->EnumerateAdapters().front();
    std::cout<<"GPU: "<<adap->GetProperties().name<<std::endl;

    NativeWindowHandle nwh; nwh.hInst=hI; nwh.hWnd=hW;
    auto surf=inst->CreateSurface(nwh);

    DeviceCreateInfo dci;
    dci.QueueRequests={{QueueType::Graphics,1,1.0f}};
    dci.CompatibleSurface=surf;
    auto dev=adap->CreateDevice(dci);

    auto sc=surf->GetCapabilities(adap);
    uint32_t imgCnt=sc.minImageCount+1;
    if(imgCnt>sc.maxImageCount&&sc.maxImageCount!=0) imgCnt=sc.maxImageCount;

    auto swap=dev->CreateSwapchain(
        SwapchainBuilder()
        .SetExtent(sc.currentExtent)
        .SetFormat(Format::BGRA8_UNORM)
        .SetColorSpace(ColorSpace::SRGB_NONLINEAR)
        .SetPresentMode(PresentMode::Fifo)
        .SetMinImageCount(imgCnt)
        .SetSurface(surf)
        .SetPreTransform(sc.currentTransform)
        .SetClipped(true)
        .Build());

    uint32_t fif=std::max(1u,swap->GetImageCount()-1);
    auto sync=dev->CreateSynchronization(fif);
    auto gq=dev->GetQueue(QueueType::Graphics,0);
    auto pq=surf->GetPresentQueue(dev);
    auto ext=swap->GetExtent();

    // Geometry
    std::vector<Vertex> verts; std::vector<uint32_t> indices;
    BuildScene(verts,indices);
    uint32_t idxCnt=(uint32_t)indices.size();

    auto vb=dev->CreateBuffer(BufferBuilder().SetSize(verts.size()*sizeof(Vertex))
        .SetUsage(BufferUsageFlags::VertexBuffer).SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(vb->Map(),verts.data(),verts.size()*sizeof(Vertex)); vb->Unmap();

    auto ib=dev->CreateBuffer(BufferBuilder().SetSize(indices.size()*4)
        .SetUsage(BufferUsageFlags::IndexBuffer).SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());
    memcpy(ib->Map(),indices.data(),indices.size()*4); ib->Unmap();

    // Shadow map textures
    auto smColor=dev->CreateTexture(TextureBuilder().SetType(TextureType::Texture2D)
        .SetSize(SM,SM).SetFormat(Format::R32_FLOAT)
        .SetUsage(TextureUsageFlags::ColorAttachment|TextureUsageFlags::Sampled).Build());

    auto smDepth=dev->CreateTexture(TextureBuilder().SetType(TextureType::Texture2D)
        .SetSize(SM,SM).SetFormat(Format::D32F)
        .SetUsage(TextureUsageFlags::DepthStencilAttachment).Build());

    auto mainDepth=dev->CreateTexture(TextureBuilder().SetType(TextureType::Texture2D)
        .SetSize(ext.width,ext.height).SetFormat(Format::D32F)
        .SetUsage(TextureUsageFlags::DepthStencilAttachment).Build());

    auto samp=dev->CreateSampler(SamplerBuilder()
        .SetFilter(Filter::Linear,Filter::Linear)
        .SetAddressMode(SamplerAddressMode::ClampToEdge)
        .SetAnisotropy(false).Build());

    // Shaders
    namespace fs=std::filesystem;
    if(fs::exists("shader_cache_shadow")) fs::remove_all("shader_cache_shadow");
    auto comp=inst->CreateShaderCompiler();
    comp->SetCacheDirectory("shader_cache_shadow");

    std::string sp="shadow_map.slang";
    { char e[MAX_PATH]={}; GetModuleFileNameA(nullptr,e,MAX_PATH);
      std::string d(e); auto s=d.find_last_of("\\/");
      if(s!=std::string::npos) d=d.substr(0,s+1);
      for(auto& c:{d+"shadow_map.slang",d+"..\\demos\\shadow_map.slang",
          std::string("demos\\shadow_map.slang"),
          std::string("E:\\Dev\\C++\\Cacao\\demos\\shadow_map.slang")})
          if(fs::exists(c)){sp=c;break;}
    }

    ShaderCreateInfo si; si.SourcePath=sp;
    si.EntryPoint="shadowVS"; si.Stage=ShaderStage::Vertex;
    auto svs=comp->CompileOrLoad(dev,si);
    si.EntryPoint="shadowPS"; si.Stage=ShaderStage::Fragment;
    auto sps=comp->CompileOrLoad(dev,si);
    si.EntryPoint="mainVS"; si.Stage=ShaderStage::Vertex;
    auto mvs=comp->CompileOrLoad(dev,si);
    si.EntryPoint="mainPS"; si.Stage=ShaderStage::Fragment;
    auto mps=comp->CompileOrLoad(dev,si);

    if(!svs||!sps||!mvs||!mps){ std::cerr<<"Shader fail\n"; return 1; }

    // Descriptor layout
    auto dl=dev->CreateDescriptorSetLayout(DescriptorSetLayoutBuilder()
        .AddBinding(0,DescriptorType::UniformBuffer,1,ShaderStage::Vertex|ShaderStage::Fragment)
        .AddBinding(1,DescriptorType::SampledImage,1,ShaderStage::Fragment)
        .AddBinding(2,DescriptorType::Sampler,1,ShaderStage::Fragment)
        .Build());
    auto pl=dev->CreatePipelineLayout(PipelineLayoutBuilder().AddSetLayout(dl).Build());

    // Shadow pipeline
    auto shadowPipe=dev->CreateGraphicsPipeline(GraphicsPipelineBuilder()
        .SetShaders({svs,sps})
        .AddVertexBinding(0,sizeof(Vertex))
        .AddVertexAttribute(0,0,Format::RGB32_FLOAT,0,"POSITION",0)
        .SetTopology(PrimitiveTopology::TriangleList)
        .SetCullMode(CullMode::None)
        .SetFrontFace(FrontFace::CounterClockwise)
        .SetDepthTest(true,true,CompareOp::Less)
        .AddColorAttachmentDefault(false)
        .AddColorFormat(Format::R32_FLOAT)
        .SetDepthStencilFormat(Format::D32F)
        .SetLayout(pl)
        .Build());

    // Main pipeline
    auto mainPipe=dev->CreateGraphicsPipeline(GraphicsPipelineBuilder()
        .SetShaders({mvs,mps})
        .AddVertexBinding(0,sizeof(Vertex))
        .AddVertexAttribute(0,0,Format::RGB32_FLOAT,0,"POSITION",0)
        .AddVertexAttribute(1,0,Format::RGB32_FLOAT,12,"NORMAL",0)
        .AddVertexAttribute(2,0,Format::RGB32_FLOAT,24,"COLOR",0)
        .SetTopology(PrimitiveTopology::TriangleList)
        .SetCullMode(CullMode::None)
        .SetFrontFace(FrontFace::CounterClockwise)
        .SetDepthTest(true,true,CompareOp::Less)
        .AddColorAttachmentDefault(false)
        .AddColorFormat(swap->GetFormat())
        .SetDepthStencilFormat(Format::D32F)
        .SetLayout(pl)
        .Build());

    if(!shadowPipe||!mainPipe){ std::cerr<<"Pipeline fail\n"; return 1; }

    // UBO + descriptors
    const uint64_t uboSz=(sizeof(SceneParams)+255)&~255;
    auto ubo=dev->CreateBuffer(BufferBuilder().SetSize(uboSz)
        .SetUsage(BufferUsageFlags::UniformBuffer).SetMemoryUsage(BufferMemoryUsage::CpuToGpu).Build());

    auto dp=dev->CreateDescriptorPool(DescriptorPoolBuilder().SetMaxSets(1)
        .AddPoolSize(DescriptorType::UniformBuffer,1)
        .AddPoolSize(DescriptorType::SampledImage,1)
        .AddPoolSize(DescriptorType::Sampler,1).Build());
    auto ds=dp->AllocateDescriptorSet(dl);

    smColor->CreateDefaultViewIfNeeded();
    ds->WriteBuffer({0,ubo,0,sizeof(SceneParams),uboSz,DescriptorType::UniformBuffer,0});
    ds->WriteTexture({1,smColor->GetDefaultView(),ResourceState::ShaderRead,DescriptorType::SampledImage,nullptr,0});
    ds->WriteSampler({2,samp,0});
    ds->Update();

    // Light
    float lp[3]={3.5f,3.0f,1.0f};
    float lv[16],lpr[16],lvp[16];
    MakeLookAt(lv,lp[0],lp[1],lp[2],-1,0,-0.5f);
    MakePerspective(lpr,2.2f,1.0f,0.3f,25.0f);
    MatMul(lpr,lv,lvp);

    // Render loop
    std::vector<Ref<CommandBufferEncoder>> cmds(fif);
    for(auto& c:cmds) c=dev->CreateCommandBufferEncoder(CommandBufferType::Primary);
    std::vector<bool> swInit(swap->GetImageCount(),false);
    uint32_t fr=0;
    MSG msg={};
    bool run=true;

    while(run)
    {
        while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){
            if(msg.message==WM_QUIT){run=false;break;}
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        if(!run) break;

        sync->WaitForFrame(fr);

        float ex=g_cx+g_dist*sinf(g_yaw)*cosf(g_pitch);
        float ey=g_cy+g_dist*sinf(g_pitch);
        float ez=g_cz+g_dist*cosf(g_yaw)*cosf(g_pitch);
        float cv[16],cp[16],cvp[16];
        MakeLookAt(cv,ex,ey,ez,g_cx,g_cy,g_cz);
        MakePerspective(cp,0.8f,(float)ext.width/ext.height,0.1f,100.0f);
        MatMul(cp,cv,cvp);

        SceneParams p;
        memcpy(p.cameraVP,cvp,64); memcpy(p.lightVP,lvp,64);
        memcpy(p.lightPos,lp,12); p._pad0=0;
        p.cameraPos[0]=ex;p.cameraPos[1]=ey;p.cameraPos[2]=ez;p._pad1=0;
        memcpy(ubo->Map(),&p,sizeof(p)); ubo->Unmap();

        int ii=0;
        if(swap->AcquireNextImage(sync,fr,ii)!=Result::Success) continue;
        sync->ResetFrameFence(fr);

        auto bb=swap->GetBackBuffer(ii);
        if(bb) bb->CreateDefaultViewIfNeeded();

        auto& cmd=cmds[fr];
        cmd->Reset(); cmd->Begin({true});

        // === Shadow pass ===
        cmd->TransitionImage(smColor,ImageTransition::UndefinedToColorAttachment);
        cmd->TransitionImage(smDepth,ImageTransition::UndefinedToDepthAttachment,{0,1,0,1,ImageAspectFlags::Depth});
        {
            smColor->CreateDefaultViewIfNeeded();
            RenderingInfo ri;
            ri.RenderArea={0,0,SM,SM};
            RenderingAttachmentInfo ca;
            ca.Texture=smColor; ca.LoadOp=AttachmentLoadOp::Clear; ca.StoreOp=AttachmentStoreOp::Store;
            ca.ClearValue=ClearValue::ColorFloat(1,1,1,1);
            ri.ColorAttachments={ca};
            auto da=std::make_shared<RenderingAttachmentInfo>();
            da->Texture=smDepth; da->LoadOp=AttachmentLoadOp::Clear; da->StoreOp=AttachmentStoreOp::DontCare;
            da->ClearDepthStencil={1.0f,0};
            ri.DepthAttachment=da;

            cmd->BeginRendering(ri);
            cmd->SetViewport({0,0,(float)SM,(float)SM,0,1});
            cmd->SetScissor({0,0,SM,SM});
            cmd->BindGraphicsPipeline(shadowPipe);
            cmd->BindVertexBuffer(0,vb,0);
            cmd->BindIndexBuffer(ib,0,IndexType::UInt32);
            std::array<Ref<DescriptorSet>,1> s1={ds};
            cmd->BindDescriptorSets(shadowPipe,0,s1);
            cmd->DrawIndexed(idxCnt,1,0,0,0);
            cmd->EndRendering();
        }
        cmd->TransitionImage(smColor,ImageTransition::ColorAttachmentToShaderRead);

        // === Main pass ===
        if(!swInit[ii]){
            cmd->TransitionImage(bb,ImageTransition::UndefinedToColorAttachment);
            swInit[ii]=true;
        } else {
            cmd->TransitionImage(bb,ImageTransition::PresentToColorAttachment);
        }
        cmd->TransitionImage(mainDepth,ImageTransition::UndefinedToDepthAttachment,{0,1,0,1,ImageAspectFlags::Depth});
        {
            RenderingInfo ri;
            ri.RenderArea={0,0,ext.width,ext.height};
            RenderingAttachmentInfo ca;
            ca.Texture=bb; ca.LoadOp=AttachmentLoadOp::Clear; ca.StoreOp=AttachmentStoreOp::Store;
            ca.ClearValue=ClearValue::ColorFloat(0.04f,0.04f,0.06f,1);
            ri.ColorAttachments={ca};
            auto da=std::make_shared<RenderingAttachmentInfo>();
            da->Texture=mainDepth; da->LoadOp=AttachmentLoadOp::Clear; da->StoreOp=AttachmentStoreOp::DontCare;
            da->ClearDepthStencil={1.0f,0};
            ri.DepthAttachment=da;

            cmd->BeginRendering(ri);
            cmd->SetViewport({0,0,(float)ext.width,(float)ext.height,0,1});
            cmd->SetScissor({0,0,ext.width,ext.height});
            cmd->BindGraphicsPipeline(mainPipe);
            cmd->BindVertexBuffer(0,vb,0);
            cmd->BindIndexBuffer(ib,0,IndexType::UInt32);
            std::array<Ref<DescriptorSet>,1> s2={ds};
            cmd->BindDescriptorSets(mainPipe,0,s2);
            cmd->DrawIndexed(idxCnt,1,0,0,0);
            cmd->EndRendering();
        }
        cmd->TransitionImage(bb,ImageTransition::ColorAttachmentToPresent);
        cmd->End();

        gq->Submit(cmd,sync,fr);
        swap->Present(pq,sync,fr);
        fr=(fr+1)%fif;
    }

    sync->WaitIdle();
    DestroyWindow(hW);
    return 0;
}
