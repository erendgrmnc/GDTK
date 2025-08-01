/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "Platform.h"

#ifdef TK_WIN
  #include "Platform/windows_main.h"
#endif
#ifdef TK_WEB
  #include "Platform/web_main.h"
#endif
#ifdef TK_ANDROID
  #include "Platform/android_main.h"
#endif

#include "Common/SDLEventPool.h"
#include "EngineSettings.h"
#include "GameRenderer.h"
#include "GameViewport.h"
#include "Plugin.h"
#include "SDL.h"
#include "Scene.h"
#include "SplashScreenRenderPath.h"
#include "TKOpenGL.h"
#include "ToolKit.h"
#include "Types.h"
#include "UIManager.h"

namespace ToolKit
{
  Game* g_game                              = nullptr;
  bool g_running                            = true;
  SDL_Window* g_window                      = nullptr;
  SDL_GLContext g_context                   = nullptr;
  Main* g_proxy                             = nullptr;
  ViewportPtr g_viewport                    = nullptr;
  EngineSettings* g_engineSettings          = nullptr;
  SDLEventPool<TK_PLATFORM>* g_sdlEventPool = nullptr;
  GameRenderer* g_gameRenderer              = nullptr;

  // Setup.
  const char* g_appName                     = "ToolKit";
  const uint g_targetFps                    = 120;

  void ProcessEvent(const SDL_Event& e)
  {
    if (e.type == SDL_QUIT)
    {
      g_running = false;
    }
  }

  void PreInit()
  {
    g_sdlEventPool = new SDLEventPool<TK_PLATFORM>();

    // PreInit Main
    g_proxy        = new Main();
    Main::SetProxy(g_proxy);

    g_proxy->PreInit();

    PlatformPreInit(g_proxy);
  }

  void Init()
  {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0)
    {
      g_running = false;
      return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    // EGL does not support sRGB backbuffer. Need to use an extension
    // https://stackoverflow.com/questions/20396523/android-egl-srgb-default-renderbuffer

    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    String settingsFile = ConcatPaths({ConfigPath(), "Engine.settings"});
    g_proxy->m_engineSettings->Load(settingsFile);
    g_engineSettings = g_proxy->m_engineSettings;
    if (g_engineSettings->m_window->GetFullScreenVal())
    {
      g_engineSettings->m_window->SetWidthVal(DM.w);
      g_engineSettings->m_window->SetHeightVal(DM.h);
    }

    PlatformAdjustEngineSettings(DM.w, DM.h, g_engineSettings);

    g_window = SDL_CreateWindow(g_appName,
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                g_engineSettings->m_window->GetWidthVal(),
                                g_engineSettings->m_window->GetHeightVal(),
                                PLATFORM_SDL_FLAGS | SDL_WINDOW_BORDERLESS);

    if (g_window == nullptr)
    {
      const char* error = SDL_GetError();
      TK_LOG("%s", error);
      g_running = false;
      return;
    }

    g_context = SDL_GL_CreateContext(g_window);
    if (g_context == nullptr)
    {
      const char* error = SDL_GetError();
      TK_LOG("%s", error);
      g_running = false;
      return;
    }

    SDL_GL_MakeCurrent(g_window, g_context);
    const char* error = SDL_GetError();
    TK_LOG("%s", error);

    // Init OpenGl.
    g_proxy->m_renderSys->InitGl((void*) SDL_GL_GetProcAddress, [](const String& msg) { TK_LOG("%s", msg.c_str()); });

    // Set defaults
    SDL_GL_SetSwapInterval(0);

    // ToolKit Init
    g_proxy->Init();

    // Register pre update function.
    TKUpdateFn preUpdateFn = [](float deltaTime)
    {
      SDL_Event sdlEvent;
      while (SDL_PollEvent(&sdlEvent))
      {
        g_sdlEventPool->PoolEvent(sdlEvent);
        ProcessEvent(sdlEvent);
      }

      // Initiate splash screen drawing.
      static bool showSplashScreen                    = true;
      static float elapsedTime                        = 0.0f;
      static SplashScreenRenderPathPtr splashRenderer = nullptr;

      if (showSplashScreen)
      {
        // Draw splash screen.
        uint width  = g_engineSettings->m_window->GetWidthVal();
        uint height = g_engineSettings->m_window->GetHeightVal();

        if (splashRenderer == nullptr)
        {
          splashRenderer = MakeNewPtr<SplashScreenRenderPath>();
          splashRenderer->Init(UVec2(width, height));
        }

        RenderSystem* rsys = GetRenderSystem();

        if (elapsedTime < 1000.0f)
        {
          elapsedTime += deltaTime;
          rsys->AddRenderTask({[](Renderer* renderer) -> void { splashRenderer->Render(renderer); }});
        }
        else
        {
          // At the end of splash drawing, initiate the app.
          rsys->AddRenderTask({[](Renderer* renderer) -> void
                               {
                                 renderer->SetFramebuffer(nullptr, GraphicBitFields::AllBits);
                                 SDL_GL_SwapWindow(g_window);
                               }});
          rsys->FlushRenderTasks();

          showSplashScreen = false;
          splashRenderer   = nullptr;

          // Init viewport and window size
          g_viewport       = MakeNewPtr<GameViewport>((float) width, (float) height);
          GetUIManager()->RegisterViewport(g_viewport);
          GetRenderSystem()->SetAppWindowSize(width, height);

          // Update widnow
          SDL_SetWindowSize(g_window, width, height);
          SDL_SetWindowBordered(g_window, SDL_TRUE);
          SDL_SetWindowResizable(g_window, SDL_TRUE);

          // Init game
          g_game = new Game();
          g_game->SetViewport(g_viewport);
          g_game->Init(g_proxy);
          g_game->m_currentState = PluginState::Running;
          g_gameRenderer         = new GameRenderer();
          g_game->OnPlay();
        }
      }
      else
      {
        // After splash shown, execute and display the app frames.
        g_viewport->Update(deltaTime);
        g_game->Frame(deltaTime);

        if (ScenePtr scene = GetSceneManager()->GetCurrentScene())
        {
          GameRendererParams params;
          params.postProcessSettings = scene->m_postProcessSettings;
          params.scene               = scene;
          params.viewport            = g_viewport;
          g_gameRenderer->SetParams(params);
        }

        GetRenderSystem()->AddRenderTask(
            {[deltaTime](Renderer* renderer) -> void { g_gameRenderer->Render(renderer); }});

        g_sdlEventPool->ClearPool(); // Clear after consumption.
        g_running = g_running && g_game->m_currentState != PluginState::Stop;
      }
    };
    g_proxy->RegisterPreUpdateFunction(preUpdateFn);

    // Register post update function.
    TKUpdateFn postUpdateFn = [](float deltaTime)
    {
      SDL_GL_MakeCurrent(g_window, g_context);
      SDL_GL_SwapWindow(g_window);

      g_sdlEventPool->ClearPool(); // Clear after consumption.
    };
    g_proxy->RegisterPostUpdateFunction(postUpdateFn);
  }

  void Exit()
  {
    SafeDel(g_gameRenderer);

    g_game->Destroy();
    Main::GetInstance()->Uninit();
    SafeDel(g_proxy);

    SafeDel(g_sdlEventPool);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    g_running = false;
  }

  void TK_Loop()
  {
    if (g_proxy->SyncFrameTime())
    {
      g_proxy->FrameBegin();
      g_proxy->FrameUpdate();
      g_proxy->FrameEnd();
    }
  }

  int ToolKit_Main(int argc, char* argv[])
  {
    PreInit();
    Init();

    PlatformMainLoop(&g_running, TK_Loop);

    Exit();
    return 0;
  }
} // namespace ToolKit

int main(int argc, char* argv[]) { return ToolKit::ToolKit_Main(argc, argv); }