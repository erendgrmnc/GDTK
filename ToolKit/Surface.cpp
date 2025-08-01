/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#include "Surface.h"

#include "Canvas.h"
#include "Material.h"
#include "Mesh.h"
#include "Node.h"
#include "Texture.h"
#include "ToolKit.h"
#include "Viewport.h"

#include "DebugNew.h"

namespace ToolKit
{

  TKDefineClass(Surface, Entity);

  Surface::Surface() {}

  Surface::~Surface() {}

  void Surface::NativeConstruct() { Super::NativeConstruct(); }

  void Surface::Update(TexturePtr texture, const Vec2& pivotOffset)
  {
    if (MaterialComponentPtr matCmp = GetMaterialComponent())
    {
      matCmp->GetFirstMaterial()->SetDiffuseTextureVal(texture);
    }

    SetPivotOffsetVal(pivotOffset);
    SetSizeFromTexture();
  }

  void Surface::Update(TexturePtr texture, const SpriteEntry& entry)
  {
    if (MaterialComponentPtr matCmp = GetMaterialComponent())
    {
      matCmp->GetFirstMaterial()->SetDiffuseTextureVal(texture);
    }

    SetSizeFromTexture();
  }

  void Surface::Update(const String& textureFile, const Vec2& pivotOffset)
  {
    if (MaterialComponentPtr matCmp = GetMaterialComponent())
    {
      TexturePtr tex = GetTextureManager()->Create<Texture>(textureFile);
      matCmp->GetFirstMaterial()->SetDiffuseTextureVal(tex);
    }

    SetPivotOffsetVal(pivotOffset);
    SetSizeFromTexture();
  }

  void Surface::Update(const Vec2& size, const Vec2& offset)
  {
    SetSizeVal(size);
    SetPivotOffsetVal(offset);
  }

  void Surface::UpdateLocalBoundingBox()
  {
    Vec2 size                   = GetSizeVal();
    Vec3 offset                 = Vec3(GetPivotOffsetVal() * size, 0.0f);
    Vec3 pos                    = ZERO;

    m_localBoundingBoxCache.min = pos + offset;
    m_localBoundingBoxCache.max = m_localBoundingBoxCache.min + Vec3(size, 0.0f);
  }

  XmlNode* Surface::SerializeImp(XmlDocument* doc, XmlNode* parent) const
  {
    XmlNode* nttNode     = Entity::SerializeImp(doc, parent);
    XmlNode* surfaceNode = CreateXmlNode(doc, StaticClass()->Name, nttNode);
    XmlNode* node        = CreateXmlNode(doc, "Anchor", surfaceNode);

    for (int i = 0; i < 4; i++)
    {
      WriteAttr(node, doc, "ratios" + std::to_string(i), std::to_string(m_anchorParams.m_anchorRatios[i]));
      WriteAttr(node, doc, "offsets" + std::to_string(i), std::to_string(m_anchorParams.m_offsets[i]));
    }

    return surfaceNode;
  }

  XmlNode* Surface::DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent)
  {
    if (info.Version >= TKV045)
    {
      return DeSerializeImpV045(info, parent);
    }

    // Old file.
    XmlNode* nttNode = Super::DeSerializeImp(info, parent);
    if (XmlNode* node = parent->first_node("Anchor"))
    {
      for (int i = 0; i < 4; i++)
      {
        ReadAttr(node, "ratios" + std::to_string(i), m_anchorParams.m_anchorRatios[i]);
        ReadAttr(node, "offsets" + std::to_string(i), m_anchorParams.m_offsets[i]);
      }
    }

    // Re assign default material.
    MaterialComponentPtr matCom = GetMaterialComponent();
    if (matCom->GetFirstMaterial()->IsDynamic())
    {
      matCom->SetFirstMaterial(GetMaterialManager()->GetCopyOfUIMaterial());
    }

    CreateQuat();

    return nttNode;
  }

  XmlNode* Surface::DeSerializeImpV045(const SerializationFileInfo& info, XmlNode* parent)
  {
    XmlNode* nttNode     = Super::DeSerializeImp(info, parent);
    XmlNode* surfaceNode = nttNode->first_node(StaticClass()->Name.c_str());

    if (XmlNode* node = surfaceNode->first_node("Anchor"))
    {
      for (int i = 0; i < 4; i++)
      {
        ReadAttr(node, "ratios" + std::to_string(i), m_anchorParams.m_anchorRatios[i]);
        ReadAttr(node, "offsets" + std::to_string(i), m_anchorParams.m_offsets[i]);
      }
    }

    // Dynamic materials are not saved, if not material is saved with surface
    // re assign default material.
    MaterialComponentPtr matCom = GetMaterialComponent();
    if (matCom != nullptr && matCom->GetMaterialList().empty())
    {
      matCom->SetFirstMaterial(GetMaterialManager()->GetCopyOfUIMaterial());
    }

    CreateQuat();

    return surfaceNode;
  }

  void Surface::UpdateGeometry(bool byTexture)
  {
    InvalidateSpatialCaches();

    if (byTexture)
    {
      SetSizeFromTexture();
    }

    MeshPtr mesh = GetMeshComponent()->GetMeshVal();
    mesh->UnInit();
    CreateQuat();
    mesh->Init();
  }

  void Surface::ComponentConstructor()
  {
    AddComponent<MeshComponent>();
    AddComponent<MaterialComponent>();

    MeshComponentPtr meshCom = GetComponent<MeshComponent>();
    meshCom->SetCastShadowVal(false);

    MaterialPtr material        = GetMaterialManager()->GetCopyOfUIMaterial();
    MaterialComponentPtr matCom = GetComponent<MaterialComponent>();
    matCom->SetFirstMaterial(material);
  }

  void Surface::ParameterConstructor()
  {
    Super::ParameterConstructor();

    Size_Define({150.0f, 50.0f}, SurfaceCategory.Name, SurfaceCategory.Priority, true, true);

    PivotOffset_Define({0.0f, 0.0f}, SurfaceCategory.Name, SurfaceCategory.Priority, true, true);

    Material_Define(GetMaterialComponent()->GetFirstMaterial(),
                    SurfaceCategory.Name,
                    SurfaceCategory.Priority,
                    true,
                    true);

    UpdateSizeFromTexture_Define(
        [this]() -> void
        {
          if (GetMaterialVal() != nullptr)
          {
            SetSizeFromTexture();
          }
        },
        SurfaceCategory.Name,
        SurfaceCategory.Priority,
        true,
        true);
  }

  void Surface::ParameterEventConstructor()
  {
    Super::ParameterEventConstructor();

    ParamSize().m_onValueChangedFn.push_back([this](Value& oldVal, Value& newVal) -> void { UpdateGeometry(false); });

    ParamPivotOffset().m_onValueChangedFn.push_back([this](Value& oldVal, Value& newVal) -> void
                                                    { UpdateGeometry(false); });

    ParamMaterial().m_onValueChangedFn.push_back(
        [this](Value& oldVal, Value& newVal) -> void
        {
          MaterialPtr material = std::get<MaterialPtr>(newVal);
          GetMaterialComponent()->SetFirstMaterial(material);
          UpdateGeometry(true);
        });
  }

  Entity* Surface::CopyTo(Entity* other) const
  {
    Entity* cpy             = Super::CopyTo(other);
    Surface* cpySurf        = static_cast<Surface*>(cpy);
    cpySurf->m_anchorParams = m_anchorParams;

    // Create an independent mesh.
    if (MeshComponentPtr meshComp = cpy->GetMeshComponent())
    {
      MeshPtr mesh = MakeNewPtr<Mesh>();
      cpy->GetMeshComponent()->SetMeshVal(mesh);

      Surface* surf = static_cast<Surface*>(cpy);
      surf->UpdateGeometry(false);
    }

    return cpy;
  }

  void Surface::ResetCallbacks()
  {
    m_onMouseEnter = nullptr;
    m_onMouseExit  = nullptr;
    m_onMouseOver  = nullptr;
    m_onMouseClick = nullptr;
  }

  void Surface::CreateQuat()
  {
    BoundingBox box = GetBoundingBox();
    Vec3 min        = box.min;
    Vec3 max        = box.max;
    float depth     = box.min.z;

    VertexArray vertices;
    vertices.resize(6);
    vertices[0].pos            = min;
    vertices[0].tex            = Vec2(0.0f, 0.0f);
    vertices[1].pos            = Vec3(max.x, min.y, depth);
    vertices[1].tex            = Vec2(1.0f, 0.0f);
    vertices[2].pos            = Vec3(max.x, max.y, depth);
    vertices[2].tex            = Vec2(1.0f, -1.0f);

    vertices[3].pos            = min;
    vertices[3].tex            = Vec2(0.0f, 0.0f);
    vertices[4].pos            = Vec3(max.x, max.y, depth);
    vertices[4].tex            = Vec2(1.0f, -1.0f);
    vertices[5].pos            = Vec3(min.x, max.y, depth);
    vertices[5].tex            = Vec2(0.0f, -1.0f);

    MeshPtr mesh               = GetMeshComponent()->GetMeshVal();
    mesh->m_clientSideVertices = vertices;
    mesh->CalculateAABB();
  }

  void Surface::CreateQuat(const SpriteEntry& val)
  {
    assert(false && "Old function needs to be re factored.");

    MeshPtr mesh      = GetMeshComponent()->GetMeshVal();
    float imageWidth  = (float) (mesh->m_material->GetDiffuseTextureVal()->m_width);
    float imageHeight = (float) (mesh->m_material->GetDiffuseTextureVal()->m_height);

    Rect<float> textureRect;
    textureRect.X      = (float) (val.rectangle.X) / (float) (imageWidth);
    textureRect.Height = ((float) (val.rectangle.Height) / (float) (imageHeight));
    textureRect.Y      = 1.0f - ((float) (val.rectangle.Y) / (float) (imageHeight)) - textureRect.Height;
    textureRect.Width  = (float) (val.rectangle.Width) / (float) (imageWidth);

    float depth        = 0.0f;
    float width        = (float) (val.rectangle.Width);
    float height       = (float) (val.rectangle.Height);
    Vec2 absOffset     = Vec2(val.offset.x * val.rectangle.Width, val.offset.y * val.rectangle.Height);

    VertexArray vertices;
    vertices.resize(6);
    vertices[0].pos            = Vec3(-absOffset.x, -absOffset.y, depth);
    vertices[0].tex            = Vec2(textureRect.X, 1.0f - textureRect.Y);
    vertices[1].pos            = Vec3(width - absOffset.x, -absOffset.y, depth);
    vertices[1].tex            = Vec2(textureRect.X + textureRect.Width, 1.0f - textureRect.Y);

    vertices[2].pos            = Vec3(-absOffset.x, height - absOffset.y, depth);
    vertices[2].tex            = Vec2(textureRect.X, 1.0f - (textureRect.Y + textureRect.Height));

    vertices[3].pos            = Vec3(width - absOffset.x, -absOffset.y, depth);
    vertices[3].tex            = Vec2(textureRect.X + textureRect.Width, 1.0f - textureRect.Y);

    vertices[4].pos            = Vec3(width - absOffset.x, height - absOffset.y, depth);
    vertices[4].tex            = Vec2(textureRect.X + textureRect.Width, 1.0f - (textureRect.Y + textureRect.Height));

    vertices[5].pos            = Vec3(-absOffset.x, height - absOffset.y, depth);
    vertices[5].tex            = Vec2(textureRect.X, 1.0f - (textureRect.Y + textureRect.Height));

    mesh->m_clientSideVertices = vertices;
    mesh->CalculateAABB();
  }

  void Surface::SetSizeFromTexture()
  {
    InvalidateSpatialCaches();

    if (MaterialPtr mat = GetMaterialVal())
    {
      if (TexturePtr dt = mat->GetDiffuseTextureVal())
      {
        SetSizeVal(Vec2(dt->m_width, dt->m_height));
      }
    }
  }

  void Surface::CalculateAnchorOffsets(Vec3 canvas[4], Vec3 surface[4])
  {

    if (canvas == nullptr || surface == nullptr)
    {
      return;
    }

    EntityPtr parentNtt = m_node->ParentEntity();
    if (parentNtt == nullptr)
    {
      return;
    }

    CanvasPtr canvasPanel = Cast<Canvas>(parentNtt);

    if (canvasPanel == nullptr)
    {
      return;
    }

    const BoundingBox& box = canvasPanel->GetBoundingBox(true);
    const float w          = box.GetWidth();
    const float h          = box.GetHeight();

    Vec3 pos               = canvasPanel->m_node->GetTranslation(TransformationSpace::TS_WORLD);
    pos.x                  = box.min.x;
    pos.y                  = box.max.y;

    const Vec3 axis[3]     = {
        {1.f, 0.f, 0.f},
        {0.f, 1.f, 0.f},
        {0.f, 0.f, 1.f}
    };

    canvas[0]                      = pos;
    canvas[0]                     -= axis[1] * ((m_anchorParams.m_anchorRatios[2]) * h);
    canvas[0]                     += axis[0] * ((m_anchorParams.m_anchorRatios[0]) * w);
    canvas[0].z                    = 0.f;

    canvas[1]                      = pos;
    canvas[1]                     -= axis[1] * ((m_anchorParams.m_anchorRatios[2]) * h);
    canvas[1]                     += axis[0] * ((1.f - m_anchorParams.m_anchorRatios[1]) * w);
    canvas[1].z                    = 0.f;

    canvas[2]                      = pos;
    canvas[2]                     -= axis[1] * ((1.f - m_anchorParams.m_anchorRatios[3]) * h);
    canvas[2]                     += axis[0] * ((m_anchorParams.m_anchorRatios[0]) * w);
    canvas[2].z                    = 0.f;

    canvas[3]                      = pos;
    canvas[3]                     -= axis[1] * ((1.f - m_anchorParams.m_anchorRatios[3]) * h);
    canvas[3]                     += axis[0] * ((1.f - m_anchorParams.m_anchorRatios[1]) * w);
    canvas[3].z                    = 0.f;

    const BoundingBox& surfaceBox  = GetBoundingBox(true);
    surface[0]                     = Vec3(surfaceBox.min.x, surfaceBox.max.y, 0.f);
    surface[1]                     = Vec3(surfaceBox.max.x, surfaceBox.max.y, 0.f);
    surface[2]                     = Vec3(surfaceBox.min.x, surfaceBox.min.y, 0.f);
    surface[3]                     = Vec3(surfaceBox.max.x, surfaceBox.min.y, 0.f);
  }

  // Button
  //////////////////////////////////////////

  TKDefineClass(Button, Surface);

  Button::Button() {}

  Button::~Button() {}

  void Button::NativeConstruct()
  {
    Super::NativeConstruct();
    ResetCallbacks();
  }

  void Button::SetBtnImage(const TexturePtr buttonImage, const TexturePtr hoverImage)
  {
    if (MaterialPtr mat = GetButtonMaterialVal())
    {
      mat->SetDiffuseTextureVal(buttonImage);
    }

    if (MaterialPtr mat = GetHoverMaterialVal())
    {
      mat->SetDiffuseTextureVal(hoverImage);
    }
  }

  void Button::ResetCallbacks()
  {
    Surface::ResetCallbacks();

    m_onMouseEnterLocal = [this](Event* e, EntityPtr ntt) -> void { SetMaterialVal(GetHoverMaterialVal()); };
    m_onMouseEnter      = m_onMouseEnterLocal;

    m_onMouseExitLocal  = [this](Event* e, EntityPtr ntt) -> void { SetMaterialVal(GetButtonMaterialVal()); };
    m_onMouseExit       = m_onMouseExitLocal;
  }

  void Button::ParameterConstructor()
  {
    Super::ParameterConstructor();

    // Update surface params.
    ParamMaterial().m_exposed     = false;
    ParamSize().m_category        = ButtonCategory;
    ParamPivotOffset().m_category = ButtonCategory;

    // Define button params.
    ButtonMaterial_Define(GetMaterialManager()->GetCopyOfUIMaterial(),
                          ButtonCategory.Name,
                          ButtonCategory.Priority,
                          true,
                          true);

    HoverMaterial_Define(GetMaterialManager()->GetCopyOfUIMaterial(),
                         ButtonCategory.Name,
                         ButtonCategory.Priority,
                         true,
                         true);
  }

  void Button::ParameterEventConstructor()
  {
    // Always rewire events for correctness.
    Surface::ParameterEventConstructor();

    ParamButtonMaterial().m_onValueChangedFn.push_back(
        [this](Value& oldVal, Value& newVal) -> void
        {
          // Override surface material.
          SetMaterialVal(std::get<MaterialPtr>(newVal));
        });
  }

  XmlNode* Button::SerializeImp(XmlDocument* doc, XmlNode* parent) const
  {
    XmlNode* root = Super::SerializeImp(doc, parent);
    XmlNode* node = CreateXmlNode(doc, StaticClass()->Name, root);

    return node;
  }

  XmlNode* Button::DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent)
  {
    if (info.Version >= TKV045)
    {
      return DeSerializeImpV045(info, parent);
    }

    // Old file.
    XmlNode* nttNode = Super::DeSerializeImp(info, parent);
    return nttNode;
  }

  XmlNode* Button::DeSerializeImpV045(const SerializationFileInfo& info, XmlNode* parent)
  {
    XmlNode* nttNode     = Super::DeSerializeImp(info, parent);
    XmlNode* surfaceNode = Surface::DeSerializeImp(info, parent);
    return surfaceNode;
  }

} // namespace ToolKit
