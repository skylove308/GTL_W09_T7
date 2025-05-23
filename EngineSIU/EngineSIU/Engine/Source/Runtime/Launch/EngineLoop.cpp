#include "EngineLoop.h"
#include "ImGuiManager.h"
#include "UnrealClient.h"
#include "WindowsPlatformTime.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/EditorEngine.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ViewportTypePanel.h"
#include "Slate/Widgets/Layout/SSplitter.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"
#include "World/World.h"
#include "Renderer/TileLightCullingPass.h"

#include "SoundManager.h"
#include "SubWindow/ImGuiSubWindow.h"
#include "SubWindow/SubCamera.h"
#include "SubWindow/SubRenderer.h"
#include "UserInterface/Drawer.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

FGraphicsDevice FEngineLoop::GraphicDevice;
FGraphicsDevice FEngineLoop::SubGraphicDevice;
FRenderer FEngineLoop::Renderer;
UPrimitiveDrawBatch FEngineLoop::PrimitiveDrawBatch;
FResourceMgr FEngineLoop::ResourceManager;
uint32 FEngineLoop::TotalAllocationBytes = 0;
uint32 FEngineLoop::TotalAllocationCount = 0;

FEngineLoop::FEngineLoop()
    : AppWnd(nullptr)
    , SubAppWnd(nullptr)
    , FUIManager(nullptr)
    , SubUI(nullptr)
    , CurrentImGuiContext(nullptr)
    , SelectedSkeletalMesh(nullptr)
    , LevelEditor(nullptr)
    , UnrealEditor(nullptr)
    , BufferManager(nullptr)
    , SubRenderer(nullptr)
{
}

int32 FEngineLoop::PreInit()
{
    return 0;
}

int32 FEngineLoop::Init(HINSTANCE hInstance)
{
    FPlatformTime::InitTiming();

    /* must be initialized before window. */
    WindowInit(hInstance);
    SubWindowInit(hInstance);

    UnrealEditor = new UnrealEd();
    BufferManager = new FDXDBufferManager();
    FUIManager = new UImGuiManager;
    AppMessageHandler = std::make_unique<FSlateAppMessageHandler>();
    LevelEditor = new SLevelEditor();

    UnrealEditor->Initialize();
    GraphicDevice.Initialize(AppWnd);
    SubRenderer = new FSubRenderer();

    if (SubAppWnd)
    {
        SubGraphicDevice.Initialize(SubAppWnd, GraphicDevice.Device);
        SubGraphicDevice.ClearColor[0] = 0.025f;
        SubGraphicDevice.ClearColor[1] = 0.025f;
        SubGraphicDevice.ClearColor[2] = 0.025f;
    }
    
    if (!GPUTimingManager.Initialize(GraphicDevice.Device, GraphicDevice.DeviceContext))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to initialize GPU Timing Manager!"));
    }
    EngineProfiler.SetGPUTimingManager(&GPUTimingManager);

    // @todo Table에 Tree 구조로 넣을 수 있도록 수정
    EngineProfiler.RegisterStatScope(TEXT("Renderer_Render"), FName(TEXT("Renderer_Render_CPU")), FName(TEXT("Renderer_Render_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- DepthPrePass"), FName(TEXT("DepthPrePass_CPU")), FName(TEXT("DepthPrePass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- TileLightCulling"), FName(TEXT("TileLightCulling_CPU")), FName(TEXT("TileLightCulling_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- ShadowPass"), FName(TEXT("ShadowPass_CPU")), FName(TEXT("ShadowPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- StaticMeshPass"), FName(TEXT("StaticMeshPass_CPU")), FName(TEXT("StaticMeshPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- WorldBillboardPass"), FName(TEXT("WorldBillboardPass_CPU")), FName(TEXT("WorldBillboardPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- UpdateLightBufferPass"), FName(TEXT("UpdateLightBufferPass_CPU")), FName(TEXT("UpdateLightBufferPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- FogPass"), FName(TEXT("FogPass_CPU")), FName(TEXT("FogPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- PostProcessCompositing"), FName(TEXT("PostProcessCompositing_CPU")), FName(TEXT("PostProcessCompositing_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- EditorBillboardPass"), FName(TEXT("EditorBillboardPass_CPU")), FName(TEXT("EditorBillboardPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- EditorRenderPass"), FName(TEXT("EditorRenderPass_CPU")), FName(TEXT("EditorRenderPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- LinePass"), FName(TEXT("LinePass_CPU")), FName(TEXT("LinePass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- GizmoPass"), FName(TEXT("GizmoPass_CPU")), FName(TEXT("GizmoPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- CompositingPass"), FName(TEXT("CompositingPass_CPU")), FName(TEXT("CompositingPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("SlatePass"), FName(TEXT("SlatePass_CPU")), FName(TEXT("SlatePass_GPU")));

    BufferManager->Initialize(GraphicDevice.Device, GraphicDevice.DeviceContext);
    Renderer.Initialize(&GraphicDevice, BufferManager, &GPUTimingManager);
    PrimitiveDrawBatch.Initialize(&GraphicDevice);
    FUIManager->Initialize(AppWnd, GraphicDevice.Device, GraphicDevice.DeviceContext);
    ResourceManager.Initialize(&Renderer, &GraphicDevice);
    
    if (SubAppWnd && SubGraphicDevice.Device) 
    {
        SubUI = new FImGuiSubWindow(SubAppWnd, SubGraphicDevice.Device, SubGraphicDevice.DeviceContext);
        UImGuiManager::ApplySharedStyle(FUIManager->GetContext(), SubUI->Context);
        SubRenderer->Initialize(&SubGraphicDevice, BufferManager);
        SubCamera = new FSubCamera(800, 600);
    }
    
    uint32 ClientWidth = 0;
    uint32 ClientHeight = 0;
    GetClientSize(ClientWidth, ClientHeight);
    LevelEditor->Initialize(ClientWidth, ClientHeight);

    GEngine = FObjectFactory::ConstructObject<UEditorEngine>(nullptr);
    GEngine->Init();


    FSoundManager::GetInstance().Initialize();
    FSoundManager::GetInstance().LoadSound("fishdream", "Contents/Sounds/fishdream.mp3");
    FSoundManager::GetInstance().LoadSound("sizzle", "Contents/Sounds/sizzle.mp3");
    //FSoundManager::GetInstance().PlaySound("fishdream");

    UpdateUI();

    return 0;
}

void FEngineLoop::Render(float DeltaTime)
{
    GraphicDevice.Prepare();
    
    if (LevelEditor->IsMultiViewport())
    {
        std::shared_ptr<FEditorViewportClient> ActiveViewportCache = GetLevelEditor()->GetActiveViewportClient();
        for (int i = 0; i < 4; ++i)
        {
            LevelEditor->SetActiveViewportClient(i);
            Renderer.Render(LevelEditor->GetActiveViewportClient());
        }
        
        for (int i = 0; i < 4; ++i)
        {
            LevelEditor->SetActiveViewportClient(i);
            Renderer.RenderViewport(LevelEditor->GetActiveViewportClient());
        }
        GetLevelEditor()->SetActiveViewportClient(ActiveViewportCache);
    }
    else
    {
        Renderer.Render(LevelEditor->GetActiveViewportClient());
        
        Renderer.RenderViewport(LevelEditor->GetActiveViewportClient());
    }


    FUIManager->BeginFrame();
    UnrealEditor->Render();
    
    FConsole::GetInstance().Draw();
    FDrawer::GetInstance().Render(DeltaTime);
    EngineProfiler.Render(GraphicDevice.DeviceContext, GraphicDevice.ScreenWidth, GraphicDevice.ScreenHeight);
    
    FUIManager->EndFrame();
}

void FEngineLoop::RenderSubWindow() const
{
    if (SubAppWnd && IsWindowVisible(SubAppWnd) && SubGraphicDevice.Device)
    {
        SubGraphicDevice.Prepare();
        
        SubRenderer->PrepareRender(*SubCamera);
        SubRenderer->Render(*SubCamera);
        SubRenderer->ClearRender();
        
        // Sub window rendering
        SubUI->BeginFrame();

        UnrealEditor->RenderSubWindowPanel();
        
        SubUI->EndFrame();
        
        // Sub swap
        SubGraphicDevice.SwapBuffer();
    }
}

void FEngineLoop::Tick()
{
    LARGE_INTEGER Frequency;
    const double TargetFrameTime = 1000.0 / TargetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

    QueryPerformanceFrequency(&Frequency);

    LARGE_INTEGER StartTime, EndTime;
    double ElapsedTime = 0.0;

    while (bIsExit == false)
    {
        FProfilerStatsManager::BeginFrame();    // Clear previous frame stats
        if (GPUTimingManager.IsInitialized())
        {
            GPUTimingManager.BeginFrame();      // Start GPU frame timing
        }

        QueryPerformanceCounter(&StartTime);

        MSG Msg;
        while (PeekMessage(&Msg, AppWnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg); // 키보드 입력 메시지를 문자메시지로 변경
            DispatchMessage(&Msg);  // 메시지를 WndProc에 전달

            if (Msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }

        // Sub window message proc
        if (!bIsExit && SubAppWnd && IsWindowVisible(SubAppWnd))
        {
            while (PeekMessage(&Msg, SubAppWnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
        // Engine loop Break
        if (bIsExit) break;

        const float DeltaTime = static_cast<float>(ElapsedTime / 1000.f);

        GEngine->Tick(DeltaTime);
        LevelEditor->Tick(DeltaTime);
        
        /** Main window render */
        Render(DeltaTime);
        
        /** Sub window render */
        RenderSubWindow();

        if (CurrentImGuiContext != nullptr)
        {
            ImGui::SetCurrentContext(CurrentImGuiContext);
        }
        
        // ImGui::SetCurrentContext(FUIManager->GetContext());
        // Pending 처리된 오브젝트 제거
        GUObjectArray.ProcessPendingDestroyObjects();

        if (GPUTimingManager.IsInitialized())
        {
            GPUTimingManager.EndFrame();        // End GPU frame timing
        }
        
        // Main swap
        GraphicDevice.SwapBuffer();


        /** Sub Window Flag */
        if (bIsShowSubWindow)
        {
            if (SubAppWnd)
            {
                ::ShowWindow(SubAppWnd, SW_SHOW);
            }
            bIsShowSubWindow = false;
        }
        
        do
        {
            Sleep(0);
            QueryPerformanceCounter(&EndTime);
            ElapsedTime = (static_cast<double>(EndTime.QuadPart - StartTime.QuadPart) * 1000.f / static_cast<double>(Frequency.QuadPart));
        } while (ElapsedTime < TargetFrameTime);
    }
}

void FEngineLoop::GetClientSize(uint32& OutWidth, uint32& OutHeight) const
{
    RECT ClientRect = {};
    GetClientRect(AppWnd, &ClientRect);
            
    OutWidth = ClientRect.right - ClientRect.left;
    OutHeight = ClientRect.bottom - ClientRect.top;
}

void FEngineLoop::Exit()
{
    if (SubGraphicDevice.Device)
    {
        SubGraphicDevice.Release();
    }
    CleanupSubWindow();

    if (FUIManager)
    {
        FUIManager->Shutdown();
        delete FUIManager;
        FUIManager = nullptr;
    }

    if (SubUI)
    {
        SubUI->Shutdown();
        delete SubUI;
        SubUI = nullptr;
    }
    
    LevelEditor->Release();
    ResourceManager.Release(&Renderer);
    Renderer.Release();
    GraphicDevice.Release();
    GEngine->Release();

    delete UnrealEditor;
    delete BufferManager;
    delete LevelEditor;
}

void FEngineLoop::CleanupSubWindow()
{
    // 서브 윈도우 리소스 해제
    if (SubGraphicDevice.Device)
    {
        SubGraphicDevice.Release();
    }
    
    if (SubAppWnd && IsWindow(SubAppWnd))
    {
        DestroyWindow(SubAppWnd);
        SubAppWnd = nullptr;
    }
}

void FEngineLoop::RequestShowWindow(bool bShow)
{
    bIsShowSubWindow = bShow;
}

void FEngineLoop::WindowInit(HINSTANCE hInstance)
{
    WCHAR WindowClass[] = L"JungleWindowClass";

    WCHAR Title[] = L"Game Tech Lab";

    WNDCLASSW wc{};
    wc.lpfnWndProc = AppWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WindowClass;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

    RegisterClassW(&wc);

    AppWnd = CreateWindowExW(
        0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 1000,
        nullptr, nullptr, hInstance, nullptr
    );
}

void FEngineLoop::SubWindowInit(HINSTANCE hInstance)
{
    WCHAR SubWindowClass[] = L"JungleSubWindowClass";
    WCHAR SubTitle[] = L"Skeleton Mesh Viewer";

    WNDCLASSEXW wcexSub = {}; // WNDCLASSEXW 사용 권장
    wcexSub.cbSize = sizeof(WNDCLASSEX);
    wcexSub.style = CS_HREDRAW | CS_VREDRAW; // | CS_DBLCLKS 등 필요시 추가
    wcexSub.lpfnWndProc = AppWndProc; // 서브 윈도우 프로시저 지정
    wcexSub.cbClsExtra = 0;
    wcexSub.cbWndExtra = 0;
    wcexSub.hInstance = hInstance;
    wcexSub.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexSub.lpszMenuName = nullptr;
    wcexSub.lpszClassName = SubWindowClass;

    if (!RegisterClassExW(&wcexSub))
    {
        // 오류 처리
        UE_LOG(ELogLevel::Error, TEXT("Failed to register sub window class!"));
        return;
    }

    // 서브 윈도우 생성 (크기, 위치, 스타일 조정 필요)
    // WS_OVERLAPPEDWINDOW 는 타이틀 바, 메뉴, 크기 조절 등이 포함된 일반적인 창
    // WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME 등으로 커스텀 가능
    SubAppWnd = CreateWindowExW(
        0, SubWindowClass, SubTitle, WS_OVERLAPPEDWINDOW, // WS_VISIBLE 제거 (초기에는 숨김)
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, // 원하는 크기
        AppWnd, // 부모 윈도우를 메인 윈도우로 설정 (선택 사항)
        nullptr, hInstance, nullptr
    );

    if (!SubAppWnd)
    {
        // 오류 처리
        UE_LOG(ELogLevel::Error, TEXT("Failed to create sub window!"));
    }
    else
    {
        // 필요할 때
        // ShowWindow(SubAppWnd, SW_SHOW);
        // 호출하여 표시
    }
}

LRESULT CALLBACK FEngineLoop::AppWndProc(HWND hWnd, uint32 Msg, WPARAM wParam, LPARAM lParam)
{
    if (hWnd == GEngineLoop.AppWnd)
    {
        ImGui::SetCurrentContext(GEngineLoop.FUIManager->GetContext());
        if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) return true;
    }
    else if (hWnd == GEngineLoop.SubAppWnd)
    {
        ImGui::SetCurrentContext(GEngineLoop.SubUI->Context);
        if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) return true;

        /** SubWindow Msg */
        switch (Msg)
        {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                RECT ClientRect;
                GetClientRect(hWnd, &ClientRect);

                float FullWidth = static_cast<float>(ClientRect.right - ClientRect.left);
                float FullHeight = static_cast<float>(ClientRect.bottom - ClientRect.top);
                
                if (GEngineLoop.GetUnrealEditor())
                {
                    SubGraphicDevice.Resize(hWnd, FullWidth, FullHeight);
                    GEngineLoop.GetUnrealEditor()->OnResize(hWnd, true);
                }
                GEngineLoop.SubCamera->UpdateCamera(FullWidth * 0.75f, FullHeight);
            }
            return 0;
        case WM_CLOSE:
            GEngineLoop.SelectSkeletalMesh(nullptr);
            GEngineLoop.SubCamera->Reset();
            ::ShowWindow(hWnd, SW_HIDE);
            return 0;
        
        case WM_ACTIVATE:
            if (ImGui::GetCurrentContext() == nullptr) break; 
            ImGui::SetCurrentContext(GEngineLoop.SubUI->Context);
            GEngineLoop.CurrentImGuiContext = ImGui::GetCurrentContext();
            return 0;

        case WM_RBUTTONDOWN:
            if (GEngineLoop.SubCamera)
            {
                SetCapture(hWnd);
                POINT MousePos;
                GetCursorPos(&MousePos);
                ScreenToClient(hWnd, &MousePos);
                GEngineLoop.SubCamera->OnMouseDownRight(MousePos.x, MousePos.y, hWnd);
            }
            
            return 0;
        case WM_RBUTTONUP:
            ReleaseCapture();
            if (GEngineLoop.SubCamera)
            {
                GEngineLoop.SubCamera->OnMouseUpRight();
            }
            return 0;

        case WM_MOUSEMOVE:
            if (GEngineLoop.SubCamera)
            {
                POINT currentMousePos;
                GetCursorPos(&currentMousePos);
                ScreenToClient(hWnd, &currentMousePos);
                
                GEngineLoop.SubCamera->OnMouseMove(currentMousePos.x, currentMousePos.y, hWnd);
            }
            return 0;
        case WM_MOUSEWHEEL:
            if (GEngineLoop.SubCamera)
            {
                short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                GEngineLoop.SubCamera->OnMouseWheel(wheelDelta);
            }
            return 0;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
        }
    }

    /** Main Window Msg */
    switch (Msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            if (auto LevelEditor = GEngineLoop.GetLevelEditor())
            {
                LevelEditor->SaveConfig();
            }
            /** Todo: 현재 PostQuitMessage의 종료 메시지가 정상적으로 수신되지 않아
             *  `bIsExit`을 강제로 true로 만들어주었습니다. 나중에 수정이 필요합니다.
             *
             *  Todo: Currently PostQuitMessage's exit message is not responded to.
             *  You can make `bIsExit` true by executing it. It will need to be fixed later.
             */
            GEngineLoop.bIsExit = true;
            break;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                if (auto LevelEditor = GEngineLoop.GetLevelEditor())
                {
                    FEngineLoop::GraphicDevice.Resize(hWnd);
                    // FEngineLoop::Renderer.DepthPrePass->ResizeDepthStencil();
                
                    uint32 ClientWidth = 0;
                    uint32 ClientHeight = 0;
                    GEngineLoop.GetClientSize(ClientWidth, ClientHeight);
            
                    LevelEditor->ResizeEditor(ClientWidth, ClientHeight);
                    FEngineLoop::Renderer.TileLightCullingPass->ResizeViewBuffers(
                      static_cast<uint32>(LevelEditor->GetActiveViewportClient()->GetD3DViewport().Width),
                        static_cast<uint32>(LevelEditor->GetActiveViewportClient()->GetD3DViewport().Height)
                    );
                }
            }
            GEngineLoop.UpdateUI();
            break;
        
        case WM_ACTIVATE:
            if (ImGui::GetCurrentContext() == nullptr) break;
            ImGui::SetCurrentContext(GEngineLoop.FUIManager->GetContext());
            GEngineLoop.CurrentImGuiContext = ImGui::GetCurrentContext();
            break;
        default:
            if (hWnd == GEngineLoop.AppWnd && GEngineLoop.AppMessageHandler != nullptr)
            {
                GEngineLoop.AppMessageHandler->ProcessMessage(hWnd, Msg, wParam, lParam);
            }
            return DefWindowProc(hWnd, Msg, wParam, lParam);
        }
    
    return 0;
}

void FEngineLoop::UpdateUI()
{
    FConsole::GetInstance().OnResize(AppWnd);
    FDrawer::GetInstance().OnResize(AppWnd);
    if (GEngineLoop.GetUnrealEditor())
    {
        GEngineLoop.GetUnrealEditor()->OnResize(AppWnd);
    }
    ViewportTypePanel::GetInstance().OnResize(AppWnd);
}

void FEngineLoop::SelectSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
    SelectedSkeletalMesh = SkeletalMesh;
    if (SubRenderer)
    {
        SubRenderer->SetPreviewSkeletalMesh(SelectedSkeletalMesh);
    }
}

void FEngineLoop::ToggleContentDrawer()
{
    FDrawer::GetInstance().Toggle();
}
