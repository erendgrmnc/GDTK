/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "OverlayLighting.h"

#include "App.h"
#include "EditorRenderer.h"
#include "SimulationWindow.h"

namespace ToolKit
{
  namespace Editor
  {

    OverlayLighting::OverlayLighting(EditorViewport* owner) : OverlayUI(owner) {}

    void OverlayLighting::Show()
    {
      Vec2 overlaySize(28.0f, 30.0f);
      if (!m_editorLitModeOn)
      {
        overlaySize.x += 170.0f;
      }

      const float padding  = 5.0f;

      // Upper right.
      Vec2 wndPos          = m_owner->m_contentAreaLocation + m_owner->m_wndContentAreaSize;
      wndPos.y             = m_owner->m_contentAreaLocation.y + overlaySize.y + padding * 2.0f;

      wndPos              -= overlaySize;
      wndPos              -= padding;

      ImGui::SetNextWindowPos(wndPos);
      ImGui::SetNextWindowBgAlpha(0.65f);

      if (ImGui::BeginChildFrame(ImGui::GetID("LightingOptions"),
                                 overlaySize,
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoScrollWithMouse))
      {
        SetOwnerState();
        ImGui::BeginTable("##SettingsBar", m_editorLitModeOn ? 1 : 2, ImGuiTableFlags_SizingStretchProp);

        ImGui::TableNextRow();
        uint nextItemIndex = 0;

        if (!m_editorLitModeOn)
        {
          ImGui::TableSetColumnIndex(nextItemIndex++);
          ImGui::PushItemWidth(160);
          uint lightModeIndx     = (uint) GetApp()->m_sceneLightingMode;
          StringView itemNames[] = {"Editor Lit", "Full Lit", "Lighting Only", "Game"};

          uint itemCount         = sizeof(itemNames) / sizeof(itemNames[0]);
          if (ImGui::BeginCombo("", itemNames[lightModeIndx].data()))
          {
            for (uint itemIndx = 1; itemIndx < itemCount; itemIndx++)
            {
              bool isSelected     = false;
              StringView itemName = itemNames[itemIndx];
              ImGui::Selectable(itemName.data(), &isSelected);
              if (isSelected)
              {
                // 0 is EditorLit
                lightModeIndx = itemIndx;
              }
            }

            ImGui::EndCombo();
          }

          ImGui::PopItemWidth();
          GetApp()->m_sceneLightingMode = (EditorLitMode) lightModeIndx;
        }
        else
        {
          if (GetApp()->m_gameMod == GameMod::Stop)
          {
            GetApp()->m_sceneLightingMode = EditorLitMode::EditorLit;
          }
        }

        ImGui::TableSetColumnIndex(nextItemIndex++);
        m_editorLitModeOn = UI::ToggleButton(ICON_FA_LIGHTBULB, ImVec2(20.0f, 20.0f), m_editorLitModeOn);

        UI::HelpMarker(TKLoc + m_owner->m_name, "Scene Lighting Mode");
        ImGui::EndTable();
      }
      ImGui::EndChildFrame();
    }

  } // namespace Editor
} // namespace ToolKit
