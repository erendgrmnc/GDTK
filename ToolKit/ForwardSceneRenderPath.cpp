﻿/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "ForwardSceneRenderPath.h"

#include "Material.h"
#include "MathUtil.h"
#include "Scene.h"
#include "Shader.h"
#include "Stats.h"

#include "DebugNew.h"

namespace ToolKit
{

  ForwardSceneRenderPath::ForwardSceneRenderPath()
  {
    m_shadowPass            = MakeNewPtr<ShadowPass>();
    m_forwardRenderPass     = MakeNewPtr<ForwardRenderPass>();
    m_skyPass               = MakeNewPtr<CubeMapPass>();
    m_forwardPreProcessPass = MakeNewPtr<ForwardPreProcessPass>();
    m_ssaoPass              = MakeNewPtr<SSAOPass>();
    m_bloomPass             = MakeNewPtr<BloomPass>();
    m_dofPass               = MakeNewPtr<DoFPass>();
    m_gammaTonemapFxaaPass  = MakeNewPtr<GammaTonemapFxaaPass>();
  }

  ForwardSceneRenderPath::~ForwardSceneRenderPath()
  {
    m_shadowPass            = nullptr;
    m_forwardRenderPass     = nullptr;
    m_skyPass               = nullptr;
    m_ssaoPass              = nullptr;
    m_forwardPreProcessPass = nullptr;
    m_bloomPass             = nullptr;
    m_dofPass               = nullptr;
    m_gammaTonemapFxaaPass  = nullptr;
  }

  void ForwardSceneRenderPath::Render(Renderer* renderer)
  {
    PreRender(renderer);

    m_passArray.clear();

    // Shadow pass
    renderer->SetShadowAtlas(Cast<Texture>(m_shadowPass->GetShadowAtlas()));
    m_passArray.push_back(m_shadowPass);

    // Forward Pre Process Pass
    if (RequiresForwardPreProcessPass())
    {
      m_passArray.push_back(m_forwardPreProcessPass);
    }

    // SSAO pass
    if (m_params.postProcessSettings->GetSSAOEnabledVal())
    {
      m_passArray.push_back(m_ssaoPass);
    }

    // Draw sky pass
    renderer->m_sky = m_sky;
    if (m_drawSky)
    {
      m_passArray.push_back(m_skyPass);
    }

    // Forward pass
    m_passArray.push_back(m_forwardRenderPass);

    // Bloom pass
    if (m_params.postProcessSettings->GetBloomEnabledVal())
    {
      m_passArray.push_back(m_bloomPass);
    }

    // Depth of field pass
    if (m_params.postProcessSettings->GetDepthOfFieldEnabledVal())
    {
      m_passArray.push_back(m_dofPass);
    }

    if (m_params.applyGammaTonemapFxaa)
    {
      if (m_gammaTonemapFxaaPass->IsEnabled())
      {
        m_passArray.push_back(m_gammaTonemapFxaaPass);
      }
    }

    RenderPath::Render(renderer);

    renderer->SetShadowAtlas(nullptr);

    PostRender(renderer);
  }

  void ForwardSceneRenderPath::PreRender(Renderer* renderer)
  {
    RenderPath::PreRender(renderer);

    SetPassParams(renderer);

    // Init / ReInit forward pre process.
    if (RequiresForwardPreProcessPass())
    {
      FramebufferSettings settings = m_params.MainFramebuffer->GetSettings();
      m_forwardPreProcessPass->InitBuffers(settings.width, settings.height, settings.multiSampleFrameBuffer);
    }
  }

  void ForwardSceneRenderPath::PostRender(Renderer* renderer) { RenderPath::PostRender(renderer); }

  void ForwardSceneRenderPath::SetPassParams(Renderer* renderer)
  {
    Frustum frustum            = ExtractFrustum(m_params.Cam->GetProjectViewMatrix(), false);
    EntityRawPtrArray entities = m_params.Scene->m_aabbTree.VolumeQuery(frustum);

    if (m_params.grid != nullptr)
    {
      entities.push_back(m_params.grid.get());
    }

    LightRawPtrArray lights;
    if (m_params.overrideLights.empty())
    {
      // Select non culled scene lights.
      MoveByType(entities, lights);

      // Collect directional lights.
      const LightRawPtrArray& directionalLights = m_params.Scene->GetDirectionalLights();
      renderer->SetDirectionalLights(directionalLights);
      m_forwardRenderPass->m_params.activeDirectionalLightCount = (int) directionalLights.size();
      for (Light* light : directionalLights)
      {
        lights.push_back(light);
      }

      // At this point directional lights may be added twice, due to ones coming from frustum cull.
      auto dirLightEndItr =
          std::partition(lights.begin(),
                         lights.end(),
                         [](Light* light) -> bool { return light->GetLightType() == Light::LightType::Directional; });

      // Make sure there is no duplicate for directionals.
      RemoveDuplicates(lights, lights.begin(), dirLightEndItr);
    }
    else
    {
      // or use override lights.
      for (LightPtr light : m_params.overrideLights)
      {
        lights.push_back(light.get());
      }

      LightRawPtrArray directionalLights                        = ToRawPtrArray(m_params.overrideLights);
      m_forwardRenderPass->m_params.activeDirectionalLightCount = (int) directionalLights.size();
      renderer->SetDirectionalLights(directionalLights);
    }

    int dirEndIndx                                   = RenderJobProcessor::PreSortLights(lights);
    const EnvironmentComponentPtrArray& environments = m_params.Scene->GetEnvironmentVolumes();
    RenderJobProcessor::CreateRenderJobs(m_renderData.jobs, entities, false, dirEndIndx, lights, environments);

    m_shadowPass->m_params.scene      = m_params.Scene;
    m_shadowPass->m_params.viewCamera = m_params.Cam;
    m_shadowPass->m_params.lights     = lights;

    RenderJobProcessor::SeperateRenderData(m_renderData, true);
    RenderJobProcessor::SortByMaterial(m_renderData);

    // Set CubeMapPass for sky.
    m_drawSky         = false;
    bool couldDrawSky = false;
    if (m_sky = m_params.Scene->GetSky())
    {
      m_sky->Init();
      if (m_drawSky = m_sky->GetDrawSkyVal())
      {
        if (m_sky->IsReadyToRender())
        {
          m_skyPass->m_params.FrameBuffer = m_params.MainFramebuffer;
          m_skyPass->m_params.Cam         = m_params.Cam;
          m_skyPass->m_params.Transform   = m_sky->m_node->GetTransform();
          m_skyPass->m_params.Material    = m_sky->GetSkyboxMaterial();
        }
        else
        {
          m_drawSky = false;
        }
      }
    }

    m_forwardRenderPass->m_params.renderData        = &m_renderData;
    m_forwardRenderPass->m_params.Cam               = m_params.Cam;
    m_forwardRenderPass->m_params.FrameBuffer       = m_params.MainFramebuffer;
    m_forwardRenderPass->m_params.clearBuffer       = GraphicBitFields::None;

    PostProcessingSettingsPtr pps                   = m_params.postProcessSettings;
    m_forwardRenderPass->m_params.SsaoTexture       = pps->GetSSAOEnabledVal() ? m_ssaoPass->m_ssaoTexture : nullptr;

    bool forwardPreProcessExist                     = RequiresForwardPreProcessPass();
    m_forwardRenderPass->m_params.hasForwardPrePass = forwardPreProcessExist;

    if (m_drawSky) // Sky pass will clear frame buffer.
    {
      if (forwardPreProcessExist) // We need to keep depth buffer for early Z pass.
      {
        m_skyPass->m_params.clearBuffer = GraphicBitFields::ColorBits;
      }
      else // Otherwise clear all.
      {
        m_skyPass->m_params.clearBuffer = GraphicBitFields::AllBits;
      }
    }
    else // Forward pass will clear frame buffer.
    {
      if (forwardPreProcessExist)
      {
        m_forwardRenderPass->m_params.clearBuffer = GraphicBitFields::ColorBits;
      }
      else
      {
        m_forwardRenderPass->m_params.clearBuffer = GraphicBitFields::AllBits;
      }
    }

    m_forwardPreProcessPass->m_params       = m_forwardRenderPass->m_params;

    m_ssaoPass->m_params.GNormalBuffer      = m_forwardPreProcessPass->m_normalRt;
    m_ssaoPass->m_params.GLinearDepthBuffer = m_forwardPreProcessPass->m_linearDepthRt;
    m_ssaoPass->m_params.Cam                = m_params.Cam;
    m_ssaoPass->m_params.Radius             = pps->GetSSAORadiusVal();
    m_ssaoPass->m_params.spread             = pps->GetSSAOSpreadVal();
    m_ssaoPass->m_params.Bias               = pps->GetSSAOBiasVal();
    m_ssaoPass->m_params.KernelSize         = pps->GetSSAOKernelSizeVal();

    m_bloomPass->m_params.FrameBuffer       = m_params.MainFramebuffer;
    m_bloomPass->m_params.intensity         = pps->GetBloomIntensityVal();
    m_bloomPass->m_params.minThreshold      = pps->GetBloomThresholdVal();
    m_bloomPass->m_params.iterationCount    = pps->GetBloomIterationCountVal();

    RenderTargetPtr atc = m_params.MainFramebuffer->GetColorAttachment(Framebuffer::Attachment::ColorAttachment0);
    m_dofPass->m_params.ColorRt                            = atc;

    m_dofPass->m_params.DepthRt                            = m_forwardPreProcessPass->m_linearDepthRt;
    m_dofPass->m_params.focusPoint                         = pps->GetFocusPointVal();
    m_dofPass->m_params.focusScale                         = pps->GetFocusScaleVal();
    m_dofPass->m_params.blurQuality                        = pps->ParamDofBlurQuality().GetEnum<DoFQuality>();

    // Post Process Pass
    bool gammaNeeded                                       = GetRenderSystem()->IsGammaCorrectionNeeded();
    m_gammaTonemapFxaaPass->m_params.enableGammaCorrection = pps->GetGammaCorrectionEnabledVal() && gammaNeeded;
    m_gammaTonemapFxaaPass->m_params.enableFxaa            = pps->GetFXAAEnabledVal();
    m_gammaTonemapFxaaPass->m_params.enableTonemapping     = pps->GetTonemappingEnabledVal();
    m_gammaTonemapFxaaPass->m_params.frameBuffer           = m_params.MainFramebuffer;
    m_gammaTonemapFxaaPass->m_params.tonemapMethod         = pps->GetTonemapperModeVal().GetEnum<TonemapMethod>();
    m_gammaTonemapFxaaPass->m_params.gamma                 = pps->GetGammaVal();

    FramebufferSettings fbs                                = m_params.MainFramebuffer->GetSettings();
    m_gammaTonemapFxaaPass->m_params.screenSize            = Vec2(fbs.width, fbs.height);
  }

  bool ForwardSceneRenderPath::RequiresForwardPreProcessPass()
  {
    bool ssaoEnabled = m_params.postProcessSettings->GetSSAOEnabledVal();
    bool dofEnabled  = m_params.postProcessSettings->GetDepthOfFieldEnabledVal();
    return ssaoEnabled || dofEnabled;
  }

} // namespace ToolKit