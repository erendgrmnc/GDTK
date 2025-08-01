/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "StatusBar.h"

#include "App.h"
#include "EditorViewport.h"

namespace ToolKit
{
  namespace Editor
  {

    StatusBar::StatusBar(EditorViewport* owner) : OverlayUI(owner) {}

    void StatusBar::Show()
    {
      // Status bar.
      Vec2 wndPadding = ImGui::GetStyle().WindowPadding;
      ImVec2 overlaySize;
      overlaySize.x  = m_owner->m_wndContentAreaSize.x - 2.0f;
      overlaySize.y  = 24;
      Vec2 pos       = m_owner->m_contentAreaLocation;

      pos.x         += 1;
      pos.y         += m_owner->m_wndContentAreaSize.y - wndPadding.y - 16.0f;
      ImGui::SetNextWindowPos(pos);
      ImGui::SetNextWindowBgAlpha(0.65f);

      if (ImGui::BeginChildFrame(ImGui::GetID("ProjectInfo"),
                                 overlaySize,
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoScrollWithMouse))
      {
        String info = "Status: ";
        ImGui::Text(info.c_str());

        // If the status message has changed.
        static String prevMsg = GetApp()->GetStatusMsg();
        String statusMsg      = GetApp()->GetStatusMsg();

        bool noTerminate      = EndsWith(statusMsg, g_statusNoTerminate);
        if (statusMsg != g_statusOk && !noTerminate)
        {
          // Hold msg for 3 sec. before switching to OK.
          static float elapsedTime  = 0.0f;
          elapsedTime              += ImGui::GetIO().DeltaTime;

          // For overlapping message updates,
          // always reset timer for the last event.
          if (prevMsg != statusMsg)
          {
            elapsedTime = 0.0f;
            prevMsg     = statusMsg;
          }

          if (elapsedTime > 3)
          {
            elapsedTime = 0.0f;
            GetApp()->SetStatusMsg(g_statusOk);
          }
        }

        // Inject status.
        ImGui::SameLine();

        if (noTerminate)
        {
          // Count 3 sec. to alternate between dots.
          static float elapsedTime    = 0.0f;
          elapsedTime                += ImGui::GetIO().DeltaTime;

          static const char* dots[4]  = {" ", " .", " ..", " ..."};
          String trimmed              = Trim(statusMsg.c_str(), g_statusNoTerminate);
          trimmed                    += dots[glm::clamp((int) glm::floor(elapsedTime), 0, 3)];

          if (elapsedTime > 4)
          {
            elapsedTime = 0.0f;
          }

          ImGui::Text(trimmed.c_str());
        }
        else
        {
          ImGui::Text(statusMsg.c_str());
        }

        ImVec2 msgSize = ImGui::CalcTextSize(statusMsg.c_str());
        float wndWidth = ImGui::GetContentRegionAvail().x;

        // If there is enough space for info.
        if (wndWidth * 0.3f > msgSize.x)
        {
          // Draw Projcet Info.
          Project prj = GetApp()->m_workspace.GetActiveProject();
          info        = "Project: " + prj.name + "Scene: " + prj.scene;
          pos         = ImGui::CalcTextSize(info.c_str());

          ImGui::SameLine((m_owner->m_wndContentAreaSize.x - pos.x) * 0.5f);
          info = "Project: " + prj.name;
          ImGui::BulletText(info.c_str());
          ImGui::SameLine();
          info = "Scene: " + prj.scene;
          ImGui::BulletText(info.c_str());

          // Draw Fps.
          String fps = "Fps: " + std::to_string(GetApp()->m_fps);
          ImGui::SameLine(m_owner->m_wndContentAreaSize.x - 70.0f);
          ImGui::Text(fps.c_str());
        }
      }
      ImGui::EndChildFrame();
    }

  } // namespace Editor
} // namespace ToolKit
