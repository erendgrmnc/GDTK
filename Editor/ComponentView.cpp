/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "ComponentView.h"

#include "Action.h"
#include "App.h"
#include "CustomDataView.h"
#include "EditorScene.h"

#include <AABBOverrideComponent.h>
#include <AnimationControllerComponent.h>
#include <EnvironmentComponent.h>
#include <Material.h>
#include <Mesh.h>

namespace ToolKit
{
  namespace Editor
  {

    void ShowMultiMaterialComponent(ComponentPtr& comp,
                                    std::function<bool(const String&)> showCompFunc,
                                    bool modifiableComp)
    {
      MaterialComponent* mmComp = (MaterialComponent*) comp.get();
      MaterialPtrArray& matList = mmComp->GetMaterialList();
      bool isOpen               = showCompFunc(MaterialComponentCategory.Name);

      if (isOpen)
      {
        ImGui::BeginDisabled(!modifiableComp);

        uint removeMaterialIndx = TK_UINT_MAX;
        for (uint i = 0; i < matList.size(); i++)
        {
          MaterialPtr& mat = matList[i];
          String path, fileName, ext;
          DecomposePath(mat->GetFile(), &path, &fileName, &ext);
          if (fileName.empty())
          {
            fileName = mat->m_name;
          }

          String uniqueName = fileName + "##" + std::to_string(i);
          ImGui::PushID(i);
          // push red color for X
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
          if (UI::ButtonDecorless(ICON_FA_TIMES, Vec2(15), false))
          {
            removeMaterialIndx = i;
          }
          ImGui::PopStyleColor();

          ImGui::SameLine();
          ImGui::EndDisabled();
          CustomDataView::ShowMaterialPtr(uniqueName, mat->GetFile(), mat, modifiableComp);
          ImGui::BeginDisabled(!modifiableComp);
          ImGui::PopID();
        }
        if (removeMaterialIndx != TK_UINT_MAX)
        {
          mmComp->RemoveMaterial(removeMaterialIndx);
        }

        if (UI::BeginCenteredTextButton("Update"))
        {
          mmComp->UpdateMaterialList();
        }
        UI::EndCenteredTextButton();
        ImGui::SameLine();
        if (ImGui::Button("Add"))
        {
          mmComp->AddMaterial(GetMaterialManager()->GetCopyOfDefaultMaterial());
        }
        UI::HelpMarker("Update", "Update material list by first MeshComponent's mesh list");

        ImGui::EndDisabled();
      }
    }

    void ComponentView::ShowAnimControllerComponent(ParameterVariant* var, ComponentPtr comp)
    {
      AnimRecordPtrMap& mref = var->GetVar<AnimRecordPtrMap>();
      String file, id;

      AnimControllerComponent* animPlayerComp = comp->As<AnimControllerComponent>();

      // If component isn't AnimationPlayerComponent, don't show variant.
      if (animPlayerComp == nullptr)
      {
        TK_ERR("AnimRecordPtrMap is for AnimationControllerComponent.");
        return;
      }

      if (animPlayerComp->GetActiveRecord())
      {
        String file;
        DecomposePath(animPlayerComp->GetActiveRecord()->m_animation->GetFile(), nullptr, &file, nullptr);

        String text = Format("Animation: %s, Duration: %f, T: %f",
                             file.c_str(),
                             animPlayerComp->GetActiveRecord()->m_animation->m_duration,
                             animPlayerComp->GetActiveRecord()->m_currentTime);

        ImGui::Text(text.c_str());
      }

      if (ImGui::BeginTable("Animation Records and Signals",
                            4,
                            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollY,
                            ImVec2(ImGui::GetWindowSize().x - 15, 200)))
      {
        float tableWdth = ImGui::GetItemRectSize().x;
        ImGui::TableSetupColumn("Animation", ImGuiTableColumnFlags_WidthStretch, tableWdth / 5.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, tableWdth / 2.5f);
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, tableWdth / 4.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, tableWdth / 20.0f);
        ImGui::TableHeadersRow();

        uint rowIndx                                       = 0;
        String removedSignalName                           = "";
        String nameUpdated                                 = "";
        std::pair<String, AnimRecordPtr> nameUpdatedPair   = {};

        static std::pair<String, AnimRecordPtr> extraTrack = std::make_pair("", MakeNewPtr<AnimRecord>());

        // Animation DropZone
        auto showAnimationDropzone = [tableWdth, file](uint& columnIndx, const std::pair<String, AnimRecordPtr>& pair)
        {
          ImGui::TableSetColumnIndex(columnIndx++);
          ImGui::SetCursorPosX(tableWdth / 25.0f);
          DropZone(static_cast<uint>(UI::m_clipIcon->m_textureId),
                   file,
                   [&pair](const DirectoryEntry& entry) -> void
                   {
                     if (GetResourceType(entry.m_ext) == Animation::StaticClass())
                     {
                       pair.second->m_animation = GetAnimationManager()->Create<Animation>(entry.GetFullPath());
                       if (pair.first.empty())
                       {
                         extraTrack.first = entry.m_fileName;
                       }
                     }
                     else
                     {
                       TK_ERR("Only animations are accepted.");
                     }
                   });
        };

        auto showSignalName =
            [&nameUpdated, &nameUpdatedPair, tableWdth](uint& columnIndx, const std::pair<String, AnimRecordPtr>& pair)
        {
          ImGui::TableSetColumnIndex(columnIndx++);
          ImGui::SetCursorPosY(ImGui::GetCursorPos().y + (ImGui::GetItemRectSize().y / 4.0f));
          ImGui::PushItemWidth((tableWdth / 2.5f) - 5.0f);
          String readOnly = pair.first;
          if (ImGui::InputText("##", &readOnly, ImGuiInputTextFlags_EnterReturnsTrue) && readOnly.length())
          {
            nameUpdated     = readOnly;
            nameUpdatedPair = pair;
          }
          ImGui::PopItemWidth();
        };
        for (auto it = mref.begin(); it != mref.end(); ++it, rowIndx++)
        {
          uint columnIndx = 0;
          ImGui::TableNextRow();
          ImGui::PushID(rowIndx);

          showAnimationDropzone(columnIndx, *it);

          // Signal Name
          showSignalName(columnIndx, *it);

          ImGui::EndDisabled();

          // Play, Pause & Stop Buttons
          ImGui::TableSetColumnIndex(columnIndx++);
          if (it->second->m_animation)
          {
            ImGui::SetCursorPosX(ImGui::GetCursorPos().x + (ImGui::GetItemRectSize().x / 10.0f));

            ImGui::SetCursorPosY(ImGui::GetCursorPos().y + (ImGui::GetItemRectSize().y / 5.0f));

            AnimRecordPtr activeRecord = animPlayerComp->GetActiveRecord();

            // Alternate between Play - Pause buttons.
            if (activeRecord == it->second && activeRecord->m_state == AnimRecord::State::Play)
            {
              if (UI::ImageButtonDecorless(UI::m_pauseIcon->m_textureId, Vec2(24, 24), false))
              {
                animPlayerComp->Pause();
              }
            }
            else if (UI::ImageButtonDecorless(UI::m_playIcon->m_textureId, Vec2(24, 24), false))
            {
              animPlayerComp->Play(it->first.c_str());
            }

            // Draw stop button always.
            ImGui::SameLine();
            if (UI::ImageButtonDecorless(UI::m_stopIcon->m_textureId, Vec2(24, 24), false))
            {
              animPlayerComp->Stop();
            }
          }

          ImGui::BeginDisabled(!var->m_editable);

          // Remove Button
          {
            ImGui::TableSetColumnIndex(columnIndx++);
            ImGui::SetCursorPosY(ImGui::GetCursorPos().y + (ImGui::GetItemRectSize().y / 4.0f));

            if (UI::ImageButtonDecorless(UI::m_closeIcon->m_textureId, Vec2(15, 15), false))
            {
              removedSignalName = it->first;
            }
          }

          ImGui::PopID();
        }

        // Show last extra track.
        uint columnIndx = 0;
        ImGui::TableNextRow();
        ImGui::PushID(rowIndx);

        showAnimationDropzone(columnIndx, extraTrack);

        // Signal Name
        showSignalName(columnIndx, extraTrack);
        ImGui::PopID();

        if (removedSignalName.length())
        {
          animPlayerComp->RemoveSignal(removedSignalName);
        }

        if (nameUpdated.length() && nameUpdatedPair.first != nameUpdated)
        {
          if (mref.find(nameUpdated) != mref.end())
          {
            TK_ERR("SignalName exists.");
          }
          else if (nameUpdatedPair.first == extraTrack.first)
          {
            extraTrack.first = nameUpdated;
          }
          else
          {
            auto node  = mref.extract(nameUpdatedPair.first);
            node.key() = nameUpdated;
            mref.insert(std::move(node));

            nameUpdated     = "";
            nameUpdatedPair = {};
          }
        }

        // If extra track is filled properly, add it to the list
        if (extraTrack.first != "" && extraTrack.second->m_animation != nullptr)
        {
          mref.insert(extraTrack);
          extraTrack.first  = "";
          extraTrack.second = MakeNewPtr<AnimRecord>();
        }

        ImGui::EndTable();
      }
    }

    bool ComponentView::ShowComponentBlock(ComponentPtr& comp, const bool modifiableComp)
    {
      VariantCategoryArray categories;
      comp->m_localData.GetCategories(categories, true, true);

      bool removeComp   = false;
      auto showCompFunc = [comp, &removeComp, modifiableComp](const String& headerName) -> bool
      {
        ImGui::PushID((int) comp->GetIdVal());
        String varName = headerName + "##" + std::to_string(modifiableComp);
        bool isOpen    = ImGui::CollapsingHeader(varName.c_str(), nullptr, ImGuiTreeNodeFlags_AllowItemOverlap);

        if (modifiableComp)
        {
          float offset = ImGui::GetContentRegionAvail().x - 30.0f;
          ImGui::SameLine(offset);
          if (UI::ButtonDecorless(ICON_FA_TIMES, // X
                                  ImVec2(15.0f, 15.0f),
                                  false) &&
              !removeComp)
          {
            GetApp()->SetStatusMsg(headerName + " " + g_statusRemoved);
            removeComp = true;
          }
        }
        ImGui::PopID();

        return isOpen;
      };

      ImGui::Indent();

      if (comp->IsA<MaterialComponent>())
      {
        ShowMultiMaterialComponent(comp, showCompFunc, modifiableComp);
      }
      else
      {
        // Show header if no categories exist.
        if (categories.empty())
        {
          showCompFunc(comp->Class()->Name);
        }
        else
        {
          // Show each parameter under corresponding category.
          for (VariantCategory& category : categories)
          {
            bool isOpen = showCompFunc(category.Name);

            if (isOpen)
            {
              ParameterVariantRawPtrArray vars;
              comp->m_localData.GetByCategory(category.Name, vars);

              for (ParameterVariant* var : vars)
              {
                bool editable = var->m_editable;
                if (!modifiableComp)
                {
                  var->m_editable = false;
                }
                ValueUpdateFn multiUpdate = CustomDataView::MultiUpdate(var, comp->Class());
                var->m_onValueChangedFn.push_back(multiUpdate);
                CustomDataView::ShowVariant(var, comp);
                var->m_onValueChangedFn.pop_back();
                if (!modifiableComp)
                {
                  var->m_editable = true;
                }
              }
            }
          }
        }
      }

      if (removeComp)
      {
        if (comp->IsA<SkeletonComponent>())
        {
          MeshComponentPtr mesh = comp->OwnerEntity()->GetComponent<MeshComponent>();

          if (mesh != nullptr && mesh->GetMeshVal()->IsSkinned())
          {
            GetApp()->SetStatusMsg(g_statusFailed);
            TK_WRN("Skeleton component is in use, it can't be removed.");
            return false;
          }
        }
      }

      ImGui::Unindent();
      return removeComp;
    }

    // ComponentView
    //////////////////////////////////////////

    ComponentView::ComponentView() : View("Component View")
    {
      m_viewID  = 3;
      m_viewIcn = UI::m_packageIcon;
    }

    ComponentView::~ComponentView() {}

    void ComponentView::Show()
    {
      m_entity      = GetApp()->GetCurrentScene()->GetCurrentSelection();
      EntityPtr ntt = m_entity.lock();

      if (ntt == nullptr)
      {
        ImGui::Text("Select an entity");
        return;
      }

      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

      UI::PushBoldFont();
      if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
      {
        UI::PopBoldFont();

        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, g_indentSpacing);
        ImGui::Indent();

        std::vector<ClassMeta*> compRemove;
        for (auto& com : ntt->GetComponentPtrArray())
        {
          ImGui::Spacing();
          if (ShowComponentBlock(com, true))
          {
            compRemove.push_back(com->Class());
          }
        }

        for (ClassMeta* Class : compRemove)
        {
          ActionManager::GetInstance()->AddAction(new DeleteComponentAction(ntt->GetComponent(Class)));
        }

        // Remove billboards if necessary.
        ScenePtr scene          = GetSceneManager()->GetCurrentScene();
        EditorScenePtr edtScene = Cast<EditorScene>(scene);
        edtScene->ValidateBillboard(ntt);

        ImGui::Separator();

        // Draw the centered button
        if (UI::BeginCenteredTextButton("Add Component"))
        {
          // Open the popup manually
          ImGui::OpenPopup("##NewComponentMenu");
        }
        UI::EndCenteredTextButton();

        // Get button rect (for positioning popup below)
        ImVec2 buttonMin = ImGui::GetItemRectMin();
        ImVec2 buttonMax = ImGui::GetItemRectMax();
        ImVec2 popupPos(buttonMin.x, buttonMax.y);

        // Set the popup position to appear right below the button
        ImGui::SetNextWindowPos(popupPos, ImGuiCond_Appearing);
        ImGui::PushItemWidth(150);

        if (ImGui::BeginPopup("##NewComponentMenu"))
        {
          App* editor         = GetApp();
          bool componentAdded = false;

          if (ImGui::MenuItem("Mesh Component"))
          {
            if (ntt->GetComponentFast<MeshComponent>() == nullptr)
            {
              ntt->AddComponent<MeshComponent>();
              componentAdded = true;

              editor->SetStatusMsg(g_successStr);
            }
            else
            {
              editor->SetStatusMsg(g_statusFailed);
              TK_WRN("Mesh Component already exists.");
            }
          }

          if (ImGui::MenuItem("Material Component"))
          {
            if (ntt->GetComponentFast<MaterialComponent>() == nullptr)
            {
              MaterialComponentPtr mmComp = ntt->AddComponent<MaterialComponent>();
              mmComp->UpdateMaterialList();
              componentAdded = true;

              editor->SetStatusMsg(g_successStr);
            }
            else
            {
              editor->SetStatusMsg(g_statusFailed);
              TK_WRN("Material Component already exists.");
            }
          }

          if (ImGui::MenuItem("Environment Component"))
          {
            if (ntt->GetComponentFast<EnvironmentComponent>() == nullptr)
            {
              // A default hdri must be given for component creation via editor.
              // Create a default hdri.
              TextureManager* texMan         = GetTextureManager();
              HdriPtr hdri                   = texMan->Create<Hdri>(texMan->GetDefaultResource(Hdri::StaticClass()));

              EnvironmentComponentPtr envCom = MakeNewPtr<EnvironmentComponent>();
              envCom->SetHdriVal(hdri);

              ntt->AddComponent(envCom);
              componentAdded = true;

              editor->SetStatusMsg(g_successStr);
            }
            else
            {
              editor->SetStatusMsg(g_statusFailed);
              TK_WRN("Environment Component already exists.");
            }
          }

          if (ImGui::MenuItem("Animation Controller Component"))
          {
            if (ntt->GetComponentFast<AnimControllerComponent>() == nullptr)
            {
              ntt->AddComponent<AnimControllerComponent>();
              componentAdded = true;
              editor->SetStatusMsg(g_successStr);
            }
            else
            {
              editor->SetStatusMsg(g_statusFailed);
              TK_WRN("Animation Controller Component already exists.");
            }
          }

          if (ImGui::MenuItem("Skeleton Component"))
          {
            if (ntt->GetComponentFast<SkeletonComponent>() == nullptr)
            {
              // Check if mesh component is skinned.
              MeshComponentPtr meshComp = ntt->GetComponent<MeshComponent>();
              if (meshComp && meshComp->GetMeshVal()->IsSkinned())
              {
                ntt->AddComponent<SkeletonComponent>();
                componentAdded = true;
                editor->SetStatusMsg(g_successStr);
              }
              else
              {
                editor->SetStatusMsg(g_statusFailed);
                TK_WRN("Skeleton Component can only be added to skinned meshes.");
              }
            }
            else
            {
              editor->SetStatusMsg(g_statusFailed);
              TK_WRN("Skeleton Component already exists.");
            }
          }

          if (ImGui::MenuItem("AABB Override Component"))
          {
            if (ntt->GetComponentFast<AABBOverrideComponent>() == nullptr)
            {
              ntt->AddComponent<AABBOverrideComponent>();
              componentAdded = true;
              editor->SetStatusMsg(g_successStr);
            }
            else
            {
              editor->SetStatusMsg(g_statusFailed);
              TK_WRN("AABB Override Component already exists.");
            }
          }

          // Create dynamic menu.
          ImGui::Separator();
          for (DynamicMenuPtr root : editor->m_customComponentsMenu)
          {
            ShowDynamicMenu(root,
                            [ntt, &componentAdded](const StringView& className) -> void
                            {
                              App* editor = GetApp();
                              if (ComponentPtr cmp = MakeNewPtrCasted<Component>(className))
                              {
                                if (ntt->GetComponent(cmp->Class()) == nullptr)
                                {
                                  ntt->AddComponent(cmp);
                                  componentAdded = true;

                                  editor->SetStatusMsg(g_successStr);
                                }
                                else
                                {
                                  editor->SetStatusMsg(g_statusFailed);
                                  TK_WRN("Component already exists: %s", className.data());
                                }
                              }
                            });
          }

          // State changed this means a new component has been added.
          if (componentAdded)
          {
            edtScene->AddBillboard(ntt);
          }

          ImGui::EndPopup();
        }
        ImGui::Unindent();

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
      }
      else
      {
        UI::PopBoldFont();
      }
    }

  } // namespace Editor
} // namespace ToolKit