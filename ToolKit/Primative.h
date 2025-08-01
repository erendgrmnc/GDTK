/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#pragma once

#include "Entity.h"
#include "RenderState.h"
#include "Types.h"

namespace ToolKit
{
  class TK_API Billboard : public Entity
  {
   public:
    struct Settings
    {
      bool lookAtCamera         = true;

      /**
       * If grater then 0, place the billboard always at approximately
       * (Difference due to RadialToPlanarDistance conversion)
       * given distance to camera.
       */
      float distanceToCamera    = 0.0f;

      /**
       * If greater then 0,
       * Fov changes due to window height changes doesn't shrink the object.
       */
      float heightInScreenSpace = 0.0f;

      /**
       * If true, staying behind of actual objects discards the pixels of the
       * billboard.
       */
      bool bypassDepthTest      = false;
    };

   public:
    TKDeclareClass(Billboard, Entity);

    Billboard();
    explicit Billboard(const Settings& settings);
    void NativeConstruct() override;
    virtual void LookAt(CameraPtr cam, float scale);

   protected:
    Entity* CopyTo(Entity* copyTo) const override;
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;

   public:
    Settings m_settings;
    Vec3 m_worldLocation;
    EntityPtr m_entity = nullptr;
  };

  class TK_API Cube final : public Entity
  {
   public:
    TKDeclareClass(Cube, Entity);

    Cube();
    void NativeConstruct() override;
    static void Generate(MeshComponentPtr meshComp, const Vec3& dimention);

   protected:
    void Generate();
    Entity* CopyTo(Entity* copyTo) const override;
    void ParameterConstructor() override;
    void ParameterEventConstructor() override;
    XmlNode* DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent) override;
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;

   public:
    TKDeclareParam(Vec3, CubeScale);

   private:
    bool m_generated = false;
  };

  typedef std::shared_ptr<Cube> CubePtr;

  class TK_API Quad final : public Entity
  {
   public:
    TKDeclareClass(Quad, Entity);

    Quad();
    void NativeConstruct() override;

   protected:
    Entity* CopyTo(Entity* copyTo) const override;
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;
    XmlNode* DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent) override;

   private:
    void Generate();
  };

  typedef std::shared_ptr<Quad> QuadPtr;

  class TK_API Sphere final : public Entity
  {
   public:
    TKDeclareClass(Sphere, Entity);

    Sphere();
    void NativeConstruct() override;
    static void Generate(MeshComponentPtr mesh, float radius, int numRing, int numSeg);

   protected:
    Entity* CopyTo(Entity* copyTo) const override;
    void ParameterConstructor() override;
    void ParameterEventConstructor() override;
    XmlNode* DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent) override;
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;

   public:
    TKDeclareParam(float, Radius);
    TKDeclareParam(int, NumRing);
    TKDeclareParam(int, NumSeg);
  };

  typedef std::shared_ptr<Sphere> SpherePtr;

  class TK_API Cone final : public Entity
  {
   public:
    TKDeclareClass(Cone, Entity);

    Cone();
    void NativeConstruct() override;
    void Generate(float height, float radius, int segBase, int segHeight);

   protected:
    Entity* CopyTo(Entity* copyTo) const override;
    void ParameterConstructor() override;
    void ParameterEventConstructor() override;

    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;
    XmlNode* DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent) override;

   private:
    void Generate();

   public:
    TKDeclareParam(float, Height);
    TKDeclareParam(float, Radius);
    TKDeclareParam(int, SegBase);
    TKDeclareParam(int, SegHeight);
  };

  typedef std::shared_ptr<Cone> ConePtr;

  class TK_API Arrow2d final : public Entity
  {
   public:
    TKDeclareClass(Arrow2d, Entity);

    Arrow2d();
    void NativeConstruct() override;
    void Generate(AxisLabel axis);

   protected:
    Entity* CopyTo(Entity* copyTo) const override;
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;

   private:
    AxisLabel m_label;
  };

  typedef std::shared_ptr<Arrow2d> Arrow2dPtr;

  class TK_API LineBatch final : public Entity
  {
   public:
    TKDeclareClass(LineBatch, Entity);

    LineBatch();
    void NativeConstruct() override;

    void Generate(const Vec3Array& linePnts, const Vec3& color, DrawType t, float lineWidth = 1.0f);

   protected:
    Entity* CopyTo(Entity* copyTo) const override;
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;
  };

  typedef std::shared_ptr<LineBatch> LineBatchPtr;

  class TK_API MeshGenerator final
  {
   public:
    static void GenerateCircleMesh(MeshPtr mesh, int numSegments, float radius);
    static void GenerateConeMesh(MeshPtr mesh, float radius, int vertexCount, float outerAngle);
    static void GenerateCube(MeshPtr mesh, Vec3 dimention);
  };
} // namespace ToolKit
