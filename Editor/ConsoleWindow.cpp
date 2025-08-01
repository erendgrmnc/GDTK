/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "ConsoleWindow.h"

#include "Action.h"
#include "App.h"
#include "EditorViewport.h"
#include "TransformMod.h"

#include <DirectionComponent.h>
#include <Drawable.h>
#include <Mesh.h>
#include <PluginManager.h>

namespace ToolKit
{
  namespace Editor
  {

    TKDefineClass(ConsoleWindow, Window);

    TagArgCIt GetTag(const String& tag, const TagArgArray& tagArgs)
    {
      for (TagArgArray::const_iterator ta = tagArgs.cbegin(); ta != tagArgs.cend(); ta++)
      {
        if (ta->first == tag)
        {
          return ta;
        }
      }

      return tagArgs.end();
    }

    bool TagExist(const String& tag, const TagArgArray& tagArgs) { return GetTag(tag, tagArgs) != tagArgs.end(); }

    void ParseVec(Vec3& vec, TagArgCIt tagIt)
    {
      int maxIndx = glm::min(static_cast<int>(tagIt->second.size()), 3);
      for (int i = 0; i < maxIndx; i++)
      {
        vec[i] = static_cast<float>(std::atof(tagIt->second[i].c_str()));
      }
    }

    // Executors
    bool BoolCheck(const TagArg& arg)
    {
      if (arg.second.empty())
      {
        return false;
      }

      if (arg.second.front() == "1")
      {
        return true;
      }

      return false;
    }

    void BoolCheck(const TagArgArray& tagArgs, bool* val)
    {
      if (tagArgs.empty())
      {
        return;
      }

      *val = BoolCheck(tagArgs.front());
    }

    void ShowPickDebugExec(TagArgArray tagArgs)
    {
      BoolCheck(tagArgs, &GetApp()->m_showPickingDebug);

      if (!GetApp()->m_showPickingDebug)
      {
        EditorScenePtr currScene = GetApp()->GetCurrentScene();
        if (GetApp()->m_dbgArrow)
        {
          currScene->RemoveEntity(GetApp()->m_dbgArrow->GetIdVal());
          GetApp()->m_dbgArrow = nullptr;
        }

        if (GetApp()->m_dbgFrustum)
        {
          currScene->RemoveEntity(GetApp()->m_dbgFrustum->GetIdVal());
          GetApp()->m_dbgFrustum = nullptr;
        }
      }
    }

    void ShowOverlayExec(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showOverlayUI); }

    void ShowOverlayAlwaysExec(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showOverlayUIAlways); }

    void ShowModTransitionsExec(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showStateTransitionsDebug); }

    void TransformInternal(TagArgArray tagArgs, bool set)
    {
      EntityPtr ntt = GetApp()->GetCurrentScene()->GetCurrentSelection();
      if (ntt == nullptr)
      {
        return;
      }

      TransformationSpace ts                        = TransformationSpace::TS_WORLD;
      TagArgArray::const_iterator transformSpaceTag = GetTag("ts", tagArgs);
      if (transformSpaceTag != tagArgs.end())
      {
        if (!transformSpaceTag->second.empty())
        {
          String tsStr = transformSpaceTag->second.front();
          if (tsStr == "world")
          {
            ts = TransformationSpace::TS_WORLD;
          }

          if (tsStr == "local")
          {
            ts = TransformationSpace::TS_LOCAL;
          }
        }
      }

      ActionManager::GetInstance()->AddAction(new TransformAction(ntt));
      bool actionApplied = false;

      for (TagArg& tagIt : tagArgs)
      {
        String tag        = tagIt.first;
        StringArray& args = tagIt.second;

        if (tag.empty())
        {
          continue;
        }

        if (args.empty())
        {
          continue;
        }

        Vec3 transfrom;
        int maxIndx = glm::min(static_cast<int>(args.size()), 3);
        for (int i = 0; i < maxIndx; i++)
        {
          transfrom[i] = (float) (std::atof(args[i].c_str()));
        }

        if (tag == "r")
        {
          Quaternion qx = glm::angleAxis(glm::radians(transfrom.x), X_AXIS);
          Quaternion qy = glm::angleAxis(glm::radians(transfrom.y), Y_AXIS);
          Quaternion qz = glm::angleAxis(glm::radians(transfrom.z), Z_AXIS);
          Quaternion q  = qz * qy * qx;

          if (set)
          {
            ntt->m_node->SetOrientation(q, ts);
          }
          else
          {
            ntt->m_node->Rotate(q, ts);
          }
          actionApplied = true;
        }
        else if (tag == "s")
        {
          if (set)
          {
            ntt->m_node->SetScale(transfrom);
          }
          else
          {
            ntt->m_node->Scale(transfrom);
          }
          actionApplied = true;
        }
        else if (tag == "t")
        {
          if (set)
          {
            ntt->m_node->SetTranslation(transfrom, ts);
          }
          else
          {
            ntt->m_node->Translate(transfrom, ts);
          }
          actionApplied = true;
        }
      }

      if (!actionApplied)
      {
        ActionManager::GetInstance()->RemoveLastAction();
      }
    }

    void SetTransformExec(TagArgArray tagArgs) { TransformInternal(tagArgs, true); }

    void TransformExec(TagArgArray tagArgs) { TransformInternal(tagArgs, false); }

    void SetCameraTransformExec(TagArgArray tagArgs)
    {
      TagArgArray::const_iterator viewportTag = GetTag("v", tagArgs);
      if (viewportTag != tagArgs.end())
      {
        if (viewportTag->second.empty()) // Tag cant be empty.
        {
          return;
        }

        if (EditorViewportPtr vp = GetApp()->GetViewport(viewportTag->second[0]))
        {
          if (CameraPtr c = vp->GetCamera())
          {
            if (viewportTag->second.size() == 2)
            {
              Node* node = c->m_node;
              if (viewportTag->second[1] == "Top")
              {
                vp->m_cameraAlignment = CameraAlignment::Top;
                Quaternion ws         = glm::angleAxis(glm::pi<float>(), -Y_AXIS) *
                                glm::angleAxis(glm::half_pi<float>(), X_AXIS) *
                                glm::angleAxis(glm::pi<float>(), Y_AXIS);
                node->SetOrientation(ws, TransformationSpace::TS_WORLD);
                if (c->IsOrtographic())
                {
                  node->SetTranslation(Vec3(0.0f, 10.0f, 0.0f), TransformationSpace::TS_WORLD);
                }
              }

              if (viewportTag->second[1] == "Front")
              {
                vp->m_cameraAlignment = CameraAlignment::Front;
                node->SetOrientation(Quaternion());
                if (c->IsOrtographic())
                {
                  node->SetTranslation(Vec3(0.0f, 0.0f, 10.0f), TransformationSpace::TS_WORLD);
                }
              }

              if (viewportTag->second[1] == "Left")
              {
                vp->m_cameraAlignment = CameraAlignment::Left;
                Quaternion ws         = glm::angleAxis(glm::half_pi<float>(), -Y_AXIS);
                node->SetOrientation(ws, TransformationSpace::TS_WORLD);
                if (c->IsOrtographic())
                {
                  node->SetTranslation(Vec3(-10.0f, 0.0f, 0.0f), TransformationSpace::TS_WORLD);
                }
              }
            }

            TagArgArray::const_iterator translateTag = GetTag("t", tagArgs);
            if (translateTag != tagArgs.end())
            {
              Vec3 translate;
              ParseVec(translate, translateTag);
              c->m_node->SetTranslation(translate, TransformationSpace::TS_WORLD);
            }
          }
        }
      }
    }

    void ResetCameraExec(TagArgArray tagArgs)
    {
      if (EditorViewportPtr vp = GetApp()->GetActiveViewport())
      {
        vp->ResetCameraToDefault();
      }
    }

    void GetTransformExec(TagArgArray tagArgs)
    {
      EntityPtr e = GetApp()->GetCurrentScene()->GetCurrentSelection();
      if (e != nullptr)
      {
        auto PrintTransform = [e](TransformationSpace ts) -> void
        {
          Quaternion q = e->m_node->GetOrientation(ts);
          Vec3 t       = e->m_node->GetTranslation(ts);
          Vec3 s       = e->m_node->GetScale();

          if (ConsoleWindowPtr cwnd = GetApp()->GetConsole())
          {
            String str = "T: " + glm::to_string(t);
            cwnd->AddLog(str);
            str = "R: " + glm::to_string(glm::degrees(glm::eulerAngles(q)));
            cwnd->AddLog(str);
            str = "S: " + glm::to_string(s);
            cwnd->AddLog(str);
          }
        };

        if (tagArgs.empty())
        {
          return;
        }
        if (tagArgs.front().second.empty())
        {
          return;
        }

        String tsStr = tagArgs.front().second.front();
        if (tsStr == "world")
        {
          PrintTransform(TransformationSpace::TS_WORLD);
        }

        if (tsStr == "local")
        {
          PrintTransform(TransformationSpace::TS_LOCAL);
        }
      }
    }

    void SetTransformOrientationExec(TagArgArray tagArgs)
    {
      if (tagArgs.empty())
      {
        return;
      }
      if (tagArgs.front().second.empty())
      {
        return;
      }

      String tsStr = tagArgs.front().second.front();
      if (tsStr == "world")
      {
        GetApp()->m_transformSpace = TransformationSpace::TS_WORLD;
      }

      if (tsStr == "local")
      {
        GetApp()->m_transformSpace = TransformationSpace::TS_LOCAL;
      }

      BaseMod* mod = ModManager::GetInstance()->m_modStack.back();
      if (TransformMod* tsm = dynamic_cast<TransformMod*>(mod))
      {
        tsm->Signal(TransformMod::m_backToStart);
      }
    }

    void ImportSlient(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_importSlient); }

    void SelectByTag(TagArgArray tagArgs)
    {
      if (tagArgs.empty())
      {
        return;
      }
      if (tagArgs.front().second.empty())
      {
        return;
      }

      String args = tagArgs.front().second.front();

      GetApp()->GetCurrentScene()->SelectByTag(args);
    }

    void LookAt(TagArgArray tagArgs)
    {
      TagArgArray::const_iterator targetTag = GetTag("t", tagArgs);
      if (targetTag != tagArgs.end())
      {
        if (targetTag->second.empty()) // Tag cant be empty.
        {
          return;
        }

        if (targetTag->second.empty())
        {
          return;
        }

        Vec3 target;
        ParseVec(target, targetTag);
        if (EditorViewportPtr vp = GetApp()->GetViewport(g_3dViewport))
        {
          vp->GetCamera()->GetComponent<DirectionComponent>()->LookAt(target);
        }
      }
    }

    void ApplyTransformToMesh(TagArgArray tagArgs)
    {
      // Caviate: A reload is neded since hardware buffers are not updated.
      // After refreshing hardware buffers,
      // transforms of the entity can be set to identity.
      EntityPtr ntt = GetApp()->GetCurrentScene()->GetCurrentSelection();
      if (ntt->IsDrawable())
      {
        MeshRawPtrArray meshes;
        ntt->GetMeshComponent()->GetMeshVal()->GetAllMeshes(meshes);

        Mat4 ts = ntt->m_node->GetTransform();

        for (Mesh* mesh : meshes)
        {
          mesh->ApplyTransform(ts);
        }

        TK_LOG("Transforms applied to %s", ntt->GetNameVal().c_str());
      }
      else
      {
        GetApp()->GetConsole()->AddLog(g_noValidEntity, LogType::Error);
      }
    }

    void SaveMesh(TagArgArray tagArgs)
    {
      EntityPtr ntt = GetApp()->GetCurrentScene()->GetCurrentSelection();
      if (ntt->IsDrawable())
      {
        TagArgCIt nameTag = GetTag("n", tagArgs);
        MeshPtr mesh      = ntt->GetMeshComponent()->GetMeshVal();

        String fileName   = mesh->GetFile();
        if (fileName.empty())
        {
          fileName = MeshPath(ntt->GetNameVal() + MESH);
        }

        if (nameTag != tagArgs.end())
        {
          fileName = MeshPath(nameTag->second.front() + MESH);
        }

        String fileBkp = mesh->GetFile();
        mesh->SetFile(fileName);
        mesh->Save(false);
        mesh->SetFile(fileBkp);
        TK_LOG("Mesh: %s saved.", fileName.c_str());
      }
      else
      {
        GetApp()->GetConsole()->AddLog(g_noValidEntity, LogType::Error);
      }
    }

    void ShowSelectionBoundary(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showSelectionBoundary); }

    void ShowGraphicsApiLogs(TagArgArray tagArgs)
    {
      if (tagArgs.empty())
      {
        return;
      }

      if (tagArgs.front().second.empty())
      {
        return;
      }

      String lvl                        = tagArgs.front().second.front();
      GetApp()->m_showGraphicsApiErrors = std::atoi(lvl.c_str());
    }

    void SetWorkspaceDir(TagArgArray tagArgs)
    {
      TagArgArray::const_iterator pathTag = GetTag(XmlNodePath.data(), tagArgs);
      if (pathTag != tagArgs.end())
      {
        String path     = pathTag->second.front();
        String manUpMsg = "You can manually update workspace directory in"
                          " 'yourInstallment/ToolKit/Resources/" +
                          g_workspaceFile + "'";
        if (CheckFile(path) && std::filesystem::is_directory(path))
        {
          // Try updating Workspace.settings
          if (GetApp()->m_workspace.SetDefaultWorkspace(path))
          {
            String info = "Your Workspace directry set to: " + path + "\n" + manUpMsg;
            GetApp()->GetConsole()->AddLog(info, LogType::Memo);
            return;
          }
        }

        String err = "There is a problem in creating workspace "
                     "directory with the given path.";
        err.append(" Projects will be saved in your installment folder.\n");
        err += manUpMsg;

        GetApp()->GetConsole()->AddLog(err, LogType::Error);
      }
    }

    void LoadPlugin(TagArgArray tagArgs)
    {
      if (tagArgs.empty())
      {
        return;
      }
      if (tagArgs.front().second.empty())
      {
        return;
      }

      String plugin = tagArgs.front().second.front();
      GetPluginManager()->Load(plugin);
    }

    void ShowShadowFrustum(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showDirectionalLightShadowFrustum); }

    void SelectAllEffectingLights(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_selectEffectingLights); }

    void CheckSceneHealth(TagArgArray tagArgs)
    {
      bool fix = false;
      BoolCheck(tagArgs, &fix);

      // Find all outliers and inf bounding boxes.
      int problemsFound = 0;
      if (ScenePtr scene = GetSceneManager()->GetCurrentScene())
      {
        auto fixProblemFn = [&problemsFound, fix, &scene](Entity* ntt, StringView msg) -> void
        {
          problemsFound++;

          String en   = ntt->GetNameVal();
          ObjectId id = ntt->GetIdVal();
          TK_WRN(msg.data(), en.c_str(), id);

          if (fix)
          {
            EntityPtr deletedNtt = scene->GetEntity(ntt->GetIdVal());
            ActionManager::GetInstance()->AddAction(new DeleteAction(deletedNtt));
          }
        };

        // Checks for invalid bb & outlier.
        RenderJobArray jobs;
        EntityRawPtrArray rawNtties = ToEntityRawPtrArray(scene->GetEntities());
        RenderJobProcessor::CreateRenderJobs(jobs, rawNtties);

        float stdev;
        Vec3 mean;
        RenderJobProcessor::CalculateStdev(jobs, stdev, mean);

        // We may want to redo deleted entities.
        if (fix)
        {
          ActionManager::GetInstance()->BeginActionGroup();
        }

        for (RenderJob& job : jobs)
        {
          if (RenderJobProcessor::IsOutlier(job, 3.0f, stdev, mean))
          {
            fixProblemFn(job.Entity, "Entity: %s ID: %llu is an outlier.");
          }

          if (!job.BoundingBox.IsValid())
          {
            fixProblemFn(job.Entity, "Entity: %s ID: %llu has invalid bounding box.");
          }
        }

        if (fix)
        {
          ActionManager::GetInstance()->GroupLastActions(problemsFound);
        }

        if (problemsFound == 0)
        {
          TK_LOG("No problems found.");
        }
        else
        {
          TK_WRN("%d problems found.", problemsFound);
        }
      }
    }

    void ShowSceneBoundary(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showSceneBoundary); }

    void ShowBVHNodes(TagArgArray tagArgs) { BoolCheck(tagArgs, &GetApp()->m_showBVHNodes); }

    void DeleteSelection(TagArgArray tagArgs)
    {
      bool isDeep = false;
      BoolCheck(tagArgs, &isDeep);

      EditorScenePtr scene = GetApp()->GetCurrentScene();

      EntityPtrArray selection;
      scene->GetSelectedEntities(selection);

      scene->RemoveEntity(selection, isDeep);
    }

    void ShowProfileTimer(TagArgArray tagArgs)
    {
      if (tagArgs.empty())
      {
        return;
      }

      for (auto& arg : tagArgs)
      {
        if (arg.first == "all")
        {
          bool val = BoolCheck(tagArgs.front());
          for (auto itr = TKStatTimerMap.begin(); itr != TKStatTimerMap.end(); itr++)
          {
            itr->second.enabled = val;
          }

          return;
        }
        else if (arg.first == "list")
        {
          for (auto itr = TKStatTimerMap.begin(); itr != TKStatTimerMap.end(); itr++)
          {
            TK_LOG("%s", itr->first.data());
          }
        }
        else if (arg.first == "reset")
        {
          for (auto itr = TKStatTimerMap.begin(); itr != TKStatTimerMap.end(); itr++)
          {
            bool isEnabled      = itr->second.enabled;
            itr->second         = {};
            itr->second.enabled = isEnabled;
          }
        }
        else
        {
          if (TKStatTimerMap.find(arg.first) != TKStatTimerMap.end())
          {
            TKStatTimerMap[arg.first].enabled = BoolCheck(arg);
          }
        }
      }
    }

    void SelectSimilar(TagArgArray tagArgs)
    {
      auto showUsage = []() { TK_WRN("call command with arg: --by <material, mesh>"); };
      if (tagArgs.empty())
      {
        showUsage();
        return;
      }

      TagArg arg = tagArgs.front();
      if (arg.first != "by")
      {
        showUsage();
        return;
      }

      if (arg.second.empty())
      {
        showUsage();
        return;
      }

      String searchBy = arg.second.front();

      if (EditorScenePtr currScene = GetApp()->GetCurrentScene())
      {
        if (EntityPtr currNtt = currScene->GetCurrentSelection())
        {
          // For now select all similar meshes.
          if (searchBy == "mesh")
          {
            if (MeshComponentPtr com = currNtt->GetMeshComponent())
            {
              if (MeshPtr mesh = com->GetMeshVal())
              {
                EntityPtrArray sameEntities = currScene->Filter(
                    [&mesh](EntityPtr ntt) -> bool
                    {
                      if (MeshComponentPtr com2 = ntt->GetMeshComponent())
                      {
                        if (mesh == com2->GetMeshVal())
                        {
                          return true;
                        }
                      }
                      return false;
                    });

                currScene->AddToSelection(sameEntities, false);
              }
            }
          }
          else if (searchBy == "material")
          {
            if (MaterialComponentPtr com = currNtt->GetMaterialComponent())
            {
              if (MaterialPtr mat = com->GetFirstMaterial())
              {
                EntityPtrArray sameEntities = currScene->Filter(
                    [&mat](EntityPtr ntt) -> bool
                    {
                      if (MaterialComponentPtr com2 = ntt->GetMaterialComponent())
                      {
                        if (mat == com2->GetFirstMaterial())
                        {
                          return true;
                        }
                      }
                      return false;
                    });

                currScene->AddToSelection(sameEntities, false);
              }
            }
          }
          else
          {
            showUsage();
            return;
          }
        }
      }
    }

    // ImGui ripoff. Portable helpers.
    static int Stricmp(const char* str1, const char* str2)
    {
      int d;
      while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1)
      {
        str1++;
        str2++;
      }
      return d;
    }

    static int Strnicmp(const char* str1, const char* str2, int n)
    {
      int d = 0;
      while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1)
      {
        str1++;
        str2++;
        n--;
      }
      return d;
    }

    static void Strtrim(char* str)
    {
      char* str_end = str + strlen(str);
      while (str_end > str && str_end[-1] == ' ')
      {
        str_end--;
        *str_end = 0;
      }
    }

    ConsoleWindow::ConsoleWindow()
    {
      m_name = g_consoleStr;

      CreateCommand(g_showPickDebugCmd, ShowPickDebugExec);
      CreateCommand(g_showOverlayUICmd, ShowOverlayExec);
      CreateCommand(g_showOverlayUIAlwaysCmd, ShowOverlayAlwaysExec);
      CreateCommand(g_showModTransitionsCmd, ShowModTransitionsExec);
      CreateCommand(g_setTransformCmd, SetTransformExec);
      CreateCommand(g_setCameraTransformCmd, SetCameraTransformExec);
      CreateCommand(g_resetCameraCmd, ResetCameraExec);
      CreateCommand(g_transformCmd, TransformExec);
      CreateCommand(g_getTransformCmd, GetTransformExec);
      CreateCommand(g_setTransformOrientationCmd, SetTransformOrientationExec);
      CreateCommand(g_importSlientCmd, ImportSlient);
      CreateCommand(g_selectByTag, SelectByTag);
      CreateCommand(g_lookAt, LookAt);
      CreateCommand(g_applyTransformToMesh, ApplyTransformToMesh);
      CreateCommand(g_saveMesh, SaveMesh);
      CreateCommand(g_showSelectionBoundary, ShowSelectionBoundary);
      CreateCommand(g_showGraphicsApiLogs, ShowGraphicsApiLogs);
      CreateCommand(g_setWorkspaceDir, SetWorkspaceDir);
      CreateCommand(g_loadPlugin, LoadPlugin);
      CreateCommand(g_showShadowFrustum, ShowShadowFrustum);
      CreateCommand(g_selectEffectingLights, SelectAllEffectingLights);
      CreateCommand(g_checkSceneHealth, CheckSceneHealth);
      CreateCommand(g_showSceneBoundary, ShowSceneBoundary);
      CreateCommand(g_showBVHNodes, ShowBVHNodes);
      CreateCommand(g_deleteSelection, DeleteSelection);
      CreateCommand(g_showProfileTimer, ShowProfileTimer);
      CreateCommand(g_selectSimilar, SelectSimilar);
    }

    ConsoleWindow::~ConsoleWindow() {}

    // 1   :
    // 12  :
    // 123 :
    // 1234:
    static String GetLineNumString(size_t line)
    {
      String lineNum   = std::to_string(++line);
      size_t numDigits = (size_t) std::log10(line);
      lineNum.append(std::max(4ull, numDigits) - numDigits, ' ');
      return lineNum + ": ";
    }

    void ConsoleWindow::Show()
    {
      if (ImGui::Begin(m_name.c_str(), &m_visible, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
      {
        HandleStates();

        // Output window.
        ImGuiStyle& style               = ImGui::GetStyle();
        const float footerHeightReserve = style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() + 2;
        ImGui::BeginChild("ScrollingRegion",
                          ImVec2(0, -footerHeightReserve),
                          false,
                          ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

        m_filter              = ToLower(m_filter);
        char itemBuffer[1024] = {0};

        m_itemLock.Lock();
        for (size_t i = 0; i < m_items.size(); i++)
        {
          const String item = m_items[i];
          if (m_filter.size() > 0 && !Utf8CaseInsensitiveSearch(item, m_filter))
          {
            continue;
          }

          String lineNum = GetLineNumString(i);
          ImGui::TextUnformatted(lineNum.c_str());
          ImGui::SameLine();

          bool popColor = false;
          if (item.find(g_memoStr) != String::npos)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, g_consoleMemoColor);
            popColor = true;
          }
          else if (item.find(g_commandStr) != String::npos)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, g_consoleCommandColor);
            popColor = true;
          }
          else if (item.find(g_warningStr) != String::npos)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, g_consoleWarningColor);
            popColor = true;
          }
          else if (item.find(g_errorStr) != String::npos)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, g_consoleErrorColor);
            popColor = true;
          }
          else // Than its a success.
          {
            ImGui::PushStyleColor(ImGuiCol_Text, g_consoleSuccessColor);
            popColor = true;
          }

          ImGui::PushID((int) i);
          ImGui::PushItemWidth(-1.0f);

          int itemLength = glm::min((int) item.length(), (int) sizeof(itemBuffer));
          strncpy(itemBuffer, item.c_str(), itemLength - 1);
          itemBuffer[itemLength - 1] = '\0';

          ImGui::InputText("##txt", itemBuffer, item.size(), ImGuiInputTextFlags_ReadOnly);
          ImGui::PopItemWidth();
          ImGui::PopID();

          if (popColor)
          {
            ImGui::PopStyleColor();
          }
        }
        m_itemLock.Unlock();

        if (m_scrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
          ImGui::SetScrollHereY(1.0f);
          m_scrollToBottom = false;
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginTable("##cmd", 3, ImGuiTableFlags_SizingFixedFit);
        ImGui::TableSetupColumn("##cmd", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##flt", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##clr");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Command window
        ImGui::PushItemWidth(-1);
        if (ImGui::InputTextWithHint(
                " Command",
                "Command",
                &m_command,
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
                    ImGuiInputTextFlags_CallbackHistory,
                [](ImGuiInputTextCallbackData* data) -> int
                { return (reinterpret_cast<ConsoleWindow*>(data->UserData))->TextEditCallback(data); },
                reinterpret_cast<void*>(this)))
        {
          ExecCommand(m_command);
          m_command      = "";
          m_reclaimFocus = true;
        }
        ImGui::PopItemWidth();

        if (m_reclaimFocus)
        {
          ImGui::SetKeyboardFocusHere(-1);
          m_reclaimFocus = false;
        }

        ImGui::TableNextColumn();

        // Filter bar
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint(" Filter", "Filter", &m_filter);
        ImGui::PopItemWidth();

        ImGui::TableNextColumn();

        if (ImGui::Button("Clear"))
        {
          ClearLog();
        }

        ImGui::EndTable();
      }
      ImGui::End();
    }

    void ConsoleWindow::AddLog(const String& log, LogType type)
    {
      String prefix;
      switch (type)
      {
      case LogType::Error:
        prefix = g_errorStr;
        break;
      case LogType::Warning:
        prefix = g_warningStr;
        break;
      case LogType::Command:
        prefix = g_commandStr;
        break;
      case LogType::Success:
        prefix = g_successStr;
        break;
      default:
        prefix = g_memoStr;
        break;
      }

      AddLog(log, prefix);
    }

    void ConsoleWindow::AddLog(const String& log, const String& tag)
    {
      String prefixed;
      if (tag.empty())
      {
        prefixed = log;
      }
      else
      {
        prefixed = "[" + tag + "] " + log;
      }
      m_scrollToBottom = true;

      m_itemLock.Lock();
      m_items.push_back(prefixed);
      m_itemLock.Unlock();

      if (m_items.size() > 1024)
      {
        ClearLog();
      }
    }

    void ConsoleWindow::ClearLog()
    {
      SpinlockGuard lock(m_itemLock);
      m_items.clear();
    }

    void ConsoleWindow::ExecCommand(const String& commandLine)
    {
      // Split command and args.
      String cmd;
      TagArgArray tagArgs;
      ParseCommandLine(commandLine, cmd, tagArgs);

      // Insert into history. First find match and delete it so it
      // can be pushed to the back. This isn't trying to be smart or optimal.
      m_historyPos = -1;
      for (int i = static_cast<int>(m_history.size()) - 1; i >= 0; i--)
      {
        if (Stricmp(m_history[i].c_str(), commandLine.c_str()) == 0)
        {
          m_history.erase(m_history.begin() + i);
          break;
        }
      }
      m_history.push_back(commandLine);

      // Process command.
      char buffer[256];
      if (m_commandExecutors.find(cmd) != m_commandExecutors.end())
      {
        AddLog(commandLine, LogType::Command);
        m_commandExecutors[cmd](tagArgs);
      }
      else
      {
        size_t len = static_cast<int>(cmd.length());
        snprintf(buffer, len + 20 + 1, "Unknown command: '%s'\n", cmd.c_str());
        AddLog(buffer, LogType::Error);
      }

      m_scrollToBottom = true;
    }

    void SplitPreserveText(const String& s, const String& sep, StringArray& v)
    {
      String arg          = s;
      const char spaceSub = static_cast<char>(26);

      size_t indx         = arg.find_first_of('"');
      size_t indx2        = arg.find_first_of('"', indx + 1);
      while (indx != String::npos && indx2 != String::npos)
      {
        std::replace(arg.begin() + indx, arg.begin() + indx2, ' ', spaceSub);
        std::swap(indx, indx2);
        indx2 = arg.find_first_of('"', indx + 1);
      }

      Split(arg, sep, v);

      for (String& val : v)
      {
        erase_if(val, [](char c) { return c == '"'; });
        std::replace(val.begin(), val.end(), spaceSub, ' ');
      }
    }

    void ConsoleWindow::ParseCommandLine(const String& commandLine, String& command, TagArgArray& tagArgs)
    {
      String args;
      size_t indx = commandLine.find_first_of(" ");
      if (indx != String::npos)
      {
        args    = commandLine.substr(indx + 1);
        command = commandLine.substr(0, indx);
      }
      else
      {
        command = commandLine;
        return;
      }

      // Single arg, no tag.
      if (args.find_first_of("--") == String::npos)
      {
        TagArg nullTag;
        StringArray values;
        SplitPreserveText(args, " ", nullTag.second);
        tagArgs.push_back(nullTag);
        return;
      }

      StringArray splits;
      Split(args, "--", splits);

      // Preserve all spaces in text.
      for (String& arg : splits)
      {
        StringArray values;
        SplitPreserveText(arg, " ", values);
        if (values.empty())
        {
          continue;
        }

        String tag;
        tag = values.front();
        pop_front(values);

        TagArg input(tag, values);
        tagArgs.push_back(input);
      }
    }

    // Mostly ripoff from the ImGui Console Example.
    int ConsoleWindow::TextEditCallback(ImGuiInputTextCallbackData* data)
    {
      char buffer[256];

      switch (data->EventFlag)
      {
      case ImGuiInputTextFlags_CallbackCompletion:
      {
        // Locate beginning of current word
        const char* word_end   = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while (word_start > data->Buf)
        {
          const char c = word_start[-1];
          if (c == ' ' || c == '\t' || c == ',' || c == ';')
          {
            break;
          }
          word_start--;
        }

        // Build a list of candidates
        StringArray candidates;
        for (size_t i = 0; i < m_commands.size(); i++)
        {
          if (Strnicmp(m_commands[i].c_str(), word_start, static_cast<int>(word_end - word_start)) == 0)
          {
            candidates.push_back(m_commands[i]);
          }
        }

        if (candidates.empty())
        {
          // No match
          sprintf(buffer, "No match for \"%.*s\"!\n", static_cast<int>(word_end - word_start), word_start);
          AddLog(buffer);
        }
        else if (candidates.size() == 1)
        {
          // Single match. Delete the beginning of the word and
          // replace it entirely so we've got nice casing
          data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
          data->InsertChars(data->CursorPos, candidates[0].c_str());
          data->InsertChars(data->CursorPos, " ");
        }
        else
        {
          // Multiple matches. Complete as much as we can,
          // so inputing "C" will complete to "CL" and display
          // "CLEAR" and "CLASSIFY"
          int match_len = static_cast<int>(word_end - word_start);
          for (;;)
          {
            int c                       = 0;
            bool all_candidates_matches = true;
            for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
            {
              if (i == 0)
              {
                c = toupper(candidates[i][match_len]);
              }
              else if (c == 0 || c != toupper(candidates[i][match_len]))
              {
                all_candidates_matches = false;
              }
            }
            if (!all_candidates_matches)
            {
              break;
            }
            match_len++;
          }

          if (match_len > 0)
          {
            data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
            data->InsertChars(data->CursorPos, candidates[0].c_str(), candidates[0].c_str() + match_len);
          }

          // List matches
          AddLog("Possible matches:\n");
          for (size_t i = 0; i < candidates.size(); i++)
          {
            size_t len = candidates[i].length();
            snprintf(buffer, len + 3 + 1, "- %s\n", candidates[i].c_str());
            AddLog(buffer);
          }
        }

        break;
      }
      case ImGuiInputTextFlags_CallbackHistory:
      {
        const int prev_history_pos = m_historyPos;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
          if (m_historyPos == -1)
          {
            m_historyPos = static_cast<int>(m_history.size()) - 1;
          }
          else if (m_historyPos > 0)
          {
            m_historyPos--;
          }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
          if (m_historyPos != -1)
          {
            if (++m_historyPos >= m_history.size())
            {
              m_historyPos = -1;
            }
          }
        }

        // A better implementation would preserve the data on
        // the current input line along with cursor position.
        if (prev_history_pos != m_historyPos)
        {
          const char* history_str = (m_historyPos >= 0) ? m_history[m_historyPos].c_str() : "";
          data->DeleteChars(0, data->BufTextLen);
          data->InsertChars(0, history_str);
        }
      }
      }

      return 0;
    }

    void ConsoleWindow::CreateCommand(const String& command, std::function<void(TagArgArray)> executor)
    {
      m_commands.push_back(command);
      m_commandExecutors[command] = executor;
    }

  } // namespace Editor
} // namespace ToolKit
