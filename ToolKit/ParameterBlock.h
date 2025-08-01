/*
 * Copyright (c) 2019-2025 OtSoftware
 * This code is licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0).
 * For more information, including options for a more permissive commercial license,
 * please visit [otyazilim.com] or contact us at [info@otyazilim.com].
 */

#pragma once

/**
 * @file ParameterBlock.h Header for ParameterVariant, ParameterBlock
 * and related structures.
 */

#include "Serialize.h"
#include "Types.h"

#include <variant>

/**
 * @def TKDeclareParam(Class, Name) Auto generates the code for accessing and
 * serialization of local data in the container Class.
 * Any class which needs managed ParameterBlocks must declare
 * ParameterBlock m_localData member. For each ParameterVariant, this macro
 * can be utilized to generate access methods for the corresponding
 * ParameterVariant.
 * @param Class One of the supported types by ParameterVariant.
 * @param Name Name of the ParameterVariant.
 */
#define TKDeclareParam(Class, Name)                                                                                    \
 private:                                                                                                              \
  int Name##_Index = -1;                                                                                               \
                                                                                                                       \
 private:                                                                                                              \
  inline void Name##_Define(Class val,                                                                                 \
                            const String& category,                                                                    \
                            int priority,                                                                              \
                            bool exposed,                                                                              \
                            bool editable,                                                                             \
                            UIHint hint = {})                                                                          \
  {                                                                                                                    \
    ParameterVariant var(val);                                                                                         \
    var.m_name     = #Name;                                                                                            \
    var.m_category = {category, priority};                                                                             \
    var.m_exposed  = exposed;                                                                                          \
    var.m_editable = editable;                                                                                         \
    var.m_hint     = hint;                                                                                             \
    if (Name##_Index == -1)                                                                                            \
    {                                                                                                                  \
      Name##_Index = (int) m_localData.m_variants.size();                                                              \
      m_localData.Add(var);                                                                                            \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      m_localData[Name##_Index] = var;                                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
 public:                                                                                                               \
  inline ParameterVariant& Param##Name() { return m_localData[Name##_Index]; }                                         \
                                                                                                                       \
 public:                                                                                                               \
  inline const Class& Get##Name##Val() const { return m_localData[Name##_Index].GetCVar<Class>(); }                    \
                                                                                                                       \
 public:                                                                                                               \
  inline void Set##Name##Val(const Class& val) { m_localData[Name##_Index] = val; }                                    \
                                                                                                                       \
 public:                                                                                                               \
  inline int Name##Index() { return Name##_Index; }

namespace ToolKit
{

  /**
   * Functions can be registered with variant and can be accessed within the
   * Framework. Functions are not serialized they have to be constructed in the
   * appropriate constructors.
   */
  typedef std::function<void()> VariantCallback;

  /**
   * Variant types.
   */
  using Value = std::variant<byte,
                             ubyte,
                             float,
                             int,
                             uint,
                             Vec3,
                             Vec4,
                             Mat3,
                             Mat4,
                             String,
                             bool,
                             ObjectId,
                             MeshPtr,
                             MaterialPtr,
                             Vec2,
                             HdriPtr,
                             AnimRecordPtrMap,
                             SkeletonPtr,
                             VariantCallback,
                             struct MultiChoiceVariant,
                             TexturePtr,
                             ShaderPtr>;

  /**
   * Value change function callback. When the Variant value has changed, all the
   * registered callbacks are called with old and new values of the parameter.
   */
  typedef std::function<void(Value& oldVal, Value& newVal)> ValueUpdateFn;

  /** Variant holding fixed choices. */
  struct MultiChoiceVariant
  {
    /** Parameter array, representing choices. */
    ParameterVariantArray Choices;

    template <typename T>
    const T& GetValue() const;

    template <typename EnumT>
    const EnumT GetEnum() const;

    struct CurrentValue
    {
      /** Index showing current selection. */
      uint Index = 0;

      void operator=(uint newIndex)
      {
        uint old = Index;
        Index    = newIndex;
        if (Callback != nullptr)
        {
          Value oldVal = old;
          Value newVal = Index;
          Callback(oldVal, newVal);
        }
      }

      /** Callback to get called upon current value change. */
      ValueUpdateFn Callback;

    } CurrentVal;
  };

  struct UIHint
  {
    bool isColor              = false;
    bool isRangeLimited       = false;
    float rangeMin            = 0.0f;
    float rangeMax            = 100.0f;
    float increment           = 0.1f;
    bool waitForTheEndOfInput = false;
  };

  /**
   * The category to group / access / sort and display the ParameterVariant.
   */
  struct VariantCategory
  {
    String Name; //!< Name of the category.
    /**
     * Priority of the category. Sorted and processed by this number within
     * every aspect of the framework. Such as Editor's Property Inspector.
     */
    int Priority = 0;
  };

  /**
   * Predefined category for user defined data.
   */
  static VariantCategory CustomDataCategory = {"Custom Data", 0};

  /**
   * A multi type object that encapsulates std::variant. The purpose of this
   * class is to provide automated functionality such as serialization, auto
   * cloning etc... This type of parameters can be recognized and properly
   * handled trough out the ToolKit framework. Such as, Editor's
   * Property Inspector. Any exposed parameter variant will be displayed under
   * the right category automatically.
   */
  class TK_API ParameterVariant : public Serializable
  {
    friend class ParameterBlock;

   public:
    /**
     * Enums for supported ParameterVariant types. These types are used for
     * type checking and serialization.
     */
    enum class VariantType
    {
      // Order is important. Don't change it.
      Byte,
      Ubyte,
      Float,
      Int,
      UInt,
      Vec3,
      Vec4,
      Mat3,
      Mat4,
      String,
      Bool,
      ObjectId,
      MeshPtr,
      MaterialPtr,
      Vec2,
      HdriPtr,
      AnimRecordPtrMap,
      SkeletonPtr,
      VariantCallback,
      MultiChoice,
      TexturePtr,
      ShaderPtr
    };

    /**
     * Default constructor that creates an int ParameterVariant with value of 0.
     */
    ParameterVariant();

    /**
     * Empty destructor.
     */
    virtual ~ParameterVariant();

    /**
     * Directly sets the new value.
     * @param newVal new value for the variant.
     */
    void SetValue(Value& newVal);

    /**
     * Default copy constructor makes a call to default assignment operator.
     */
    ParameterVariant(const ParameterVariant& other);

    /**
     * Default move constructor. Moves everything not like assignment.
     */
    ParameterVariant(ParameterVariant&& other) noexcept;

    /**
     * Constructs bool type variant.
     */
    explicit ParameterVariant(bool var);

    /**
     * Constructs byte type variant.
     */
    explicit ParameterVariant(byte var);

    /**
     * Constructs ubyte type variant.
     */
    explicit ParameterVariant(ubyte var);

    /**
     * Constructs float type variant.
     */
    explicit ParameterVariant(float var);

    /**
     * Constructs Vec3 type variant.
     */
    explicit ParameterVariant(int var);

    /**
     * Constructs uint type variant.
     */
    explicit ParameterVariant(uint var);

    /**
     * Constructs Vec2 type variant.
     */
    explicit ParameterVariant(const Vec2& var);

    /**
     * Constructs Vec3 type variant.
     */
    explicit ParameterVariant(const Vec3& var);

    /**
     * Constructs Vec4 type variant.
     */
    explicit ParameterVariant(const Vec4& var);

    /**
     * Constructs Mat3 type variant.
     */
    explicit ParameterVariant(const Mat3& var);

    /**
     * Constructs Mat4 type variant.
     */
    explicit ParameterVariant(const Mat4& var);

    /**
     * Constructs String type variant.
     */
    explicit ParameterVariant(const String& var);

    /**
     * Constructs const char* type variant.
     */
    explicit ParameterVariant(const char* var);

    /**
     * Constructs ObjectId type variant.
     */
    explicit ParameterVariant(ObjectId& var);

    /**
     * Constructs MeshPtr type variant.
     */
    explicit ParameterVariant(const MeshPtr& var);

    /**
     * Constructs TexturePtr type variant.
     */
    explicit ParameterVariant(const TexturePtr& var);

    /**
     * Constructs ShaderPtr type variant.
     */
    explicit ParameterVariant(const ShaderPtr& var);

    /**
     * Constructs MaterialPtr type variant.
     */
    explicit ParameterVariant(const MaterialPtr& var);

    /**
     * Constructs HdriPtr type variant.
     */
    explicit ParameterVariant(const HdriPtr& var);

    /**
     * Constructs AnimRecordPtrMap type variant.
     */
    explicit ParameterVariant(const AnimRecordPtrMap& var);

    /**
     * Constructs SkeletonPtr type variant.
     */
    explicit ParameterVariant(const SkeletonPtr& var);

    /**
     * Constructs CallbackFn type variant.
     */
    ParameterVariant(const VariantCallback& var);

    /**
     * Constructs MultiChoiceVariant type variant.
     */
    ParameterVariant(const MultiChoiceVariant& var);

    /**
     * Used to retrieve VariantType of the variant.
     * @return VariantType That corresponds to current type of the variant.
     */
    VariantType GetType() const;

    /**
     * Used to access the underlying value of the variant.
     * @return A reference to the value set during the initialization of the
     * variant.
     */
    template <typename T>
    T& GetVar()
    {
      return std::get<T>(m_var);
    }

    /**
     * Returns the const reference to the value of the variant.
     * @see T& GetVar()
     * @return const reference to the value of variant.
     */
    template <typename T>
    const T& GetCVar() const
    {
      return std::get<T>(m_var);
    }

    /**
     * Returns the pointer to the value of the variant.
     * @return A pointer to the value of the variant.
     */
    template <typename T>
    T* GetVarPtr()
    {
      return &std::get<T>(m_var);
    }

    /** Helper function to set an enum. Value internally stored as int. */
    template <typename EnumT>
    void SetEnum(EnumT e)
    {
      static_assert(std::is_enum_v<EnumT>, "EnumT must be an enum type");
      AsignVal(static_cast<int>(e));
    }

    /** Helper function to get an enum. Internally stored int casted to target EnumT. */
    template <typename EnumT>
    EnumT GetEnum()
    {
      static_assert(std::is_enum_v<EnumT>, "EnumT must be an enum type");
      return static_cast<EnumT>(GetCVar<int>());
    }

    /**
     * Default move operator, copies every member but event callbacks.
     */
    ParameterVariant& operator=(ParameterVariant&& other) noexcept;

    /**
     * Default assignment operator, copies every member but event callbacks.
     * Events refer to objects to operate on. Consider coping or
     * rewiring event callbacks explicitly. Otherwise unintended objects gets
     * affected.
     */
    ParameterVariant& operator=(const ParameterVariant& other) noexcept;

    /**
     * Assign a bool to the value of the variant.
     */
    ParameterVariant& operator=(bool var);

    /**
     * Assign a byte to the value of the variant.
     */
    ParameterVariant& operator=(byte var);

    /**
     * Assign a ubyte to the value of the variant.
     */
    ParameterVariant& operator=(ubyte var);

    /**
     * Assign a float to the value of the variant.
     */
    ParameterVariant& operator=(float var);

    /**
     * Assign a int to the value of the variant.
     */
    ParameterVariant& operator=(int var);

    /**
     * Assign a uint to the value of the variant.
     */
    ParameterVariant& operator=(uint var);

    /**
     * Assign a Vec2 to the value of the variant.
     */
    ParameterVariant& operator=(const Vec2& var);

    /**
     * Assign a Vec3 to the value of the variant.
     */
    ParameterVariant& operator=(const Vec3& var);

    /**
     * Assign a Vec4 to the value of the variant.
     */
    ParameterVariant& operator=(const Vec4& var);

    /**
     * Assign a Mat3 to the value of the variant.
     */
    ParameterVariant& operator=(const Mat3& var);

    /**
     * Assign a Mat4 to the value of the variant.
     */
    ParameterVariant& operator=(const Mat4& var);

    /**
     * Assign a String to the value of the variant.
     */
    ParameterVariant& operator=(const String& var);

    /**
     * Assign a const char* to the value of the variant.
     */
    ParameterVariant& operator=(const char* var);

    /**
     * Assign a ObjectId to the value of the variant.
     */
    ParameterVariant& operator=(ObjectId var);

    /**
     * Assign a MeshPtr to the value of the variant.
     */
    ParameterVariant& operator=(const MeshPtr& var);

    /**
     * Assign a TexturePtr to the value of the variant.
     */
    ParameterVariant& operator=(const TexturePtr& var);

    /**
     * Assign a TexturePtr to the value of the variant.
     */
    ParameterVariant& operator=(const ShaderPtr& var);

    /**
     * Assign a MaterialPtr to the value of the variant.
     */
    ParameterVariant& operator=(const MaterialPtr& var);

    /**
     * Assign a MaterialPtr to the value of the variant.
     */
    ParameterVariant& operator=(const HdriPtr& var);

    /**
     * Assign a AnimRecordPtrMap to the value of the variant.
     */
    ParameterVariant& operator=(const AnimRecordPtrMap& var);

    /**
     * Assign a SkeletonPtr to the value of the variant.
     */
    ParameterVariant& operator=(const SkeletonPtr& var);

    /**
     * Assign a CallbackFn to the value of the variant.
     */
    ParameterVariant& operator=(const VariantCallback& var);

    /**
     * Assign a MultiChoiceVariant to the value of the variant.
     */
    ParameterVariant& operator=(const MultiChoiceVariant& var);

    /**
     * Serializes the variant to the xml document.
     * @param doc The xml document object to serialize to.
     * @param parent The parent xml node to serialize to.
     */
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;

    /**
     * De serializes the variant from the xml document.
     * @param doc The xml document object to read from.
     * @param parent The parent xml node to read from.
     */
    XmlNode* DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent) override;

   public:
    /**
     * States if this variant exposed to framework / editor.
     */
    bool m_exposed;

    /**
     * States if this variable can be edited from framework / editor.
     * Does not provide explicit protection. The system that uses the variant
     * may chose to obey.
     */
    bool m_editable;

    /**
     * Framework accumulates and treats similarly to every variant that shares
     * the same category. Such as editor, it displays every exposed variant that
     * shares the same category under the same drop-down area.
     */
    VariantCategory m_category;
    String m_name = "NoName"; //<! Name of the variant.

    UIHint m_hint;

    /**
     * Callback function for value changes. This function gets called after
     * new value set.
     */
    std::vector<ValueUpdateFn> m_onValueChangedFn;

   private:
    template <typename T>
    void AsignVal(T&& val)
    {
      Value oldVal = m_var;
      m_var        = std::forward<T>(val);

      for (ValueUpdateFn& fn : m_onValueChangedFn)
      {
        fn(oldVal, m_var);
      }
    }

   private:
    Value m_var; //!< The variant that hold the actual data.

    VariantType m_type = VariantType::Int; //!< Type of the variant.
  };

  /**
   * A class that can be used to group ParameterVariant objects.
   * Act like a manager class for a group of ParameterVariant objects.
   */
  class TK_API ParameterBlock : public Serializable
  {
   public:
    /**
     * Used to access ParameterVariant's by index.
     * @return Reference to indexed ParameterVariant.
     */
    ParameterVariant& operator[](size_t index);

    /**
     * Used to access ParameterVariant's by index.
     * @return const Reference to indexed ParameterVariant.
     */
    const ParameterVariant& operator[](size_t index) const;

    /**
     * Adds a variant to the ParameterBlock. No uniqueness guaranteed.
     * @param var The ParameterVariant to insert.
     */
    void Add(const ParameterVariant& var);

    /**
     * Remove's the variant at the given index.
     * @param index of the variant to remove.
     */
    void Remove(int index);

    /**
     * Collects all unique categories and sorts the categories in
     * descending order by request.
     * @param categories An array to return unique categories.
     * @param sortDesc Sorts the categories by priority in descending order.
     * @param filterByExpose Filters out categories which does not contains any
     * exposed Variants.
     */
    void GetCategories(VariantCategoryArray& categories, bool sortDesc, bool filterByExpose);

    /**
     * Collects every variant by the given category.
     * @param category The category to search the variants in.
     * @param variants The resulting variant array which holds references to the
     * variants that falls under the requested category.
     */
    void GetByCategory(const String& category, ParameterVariantRawPtrArray& variants);

    /**
     * Collects every variant by the given category.
     * @param category The category to search the variants in.
     * @param variants Index list of variants that is under the requested category.
     */
    void GetByCategory(const String& category, IntArray& variants);

    /**
     * Search the variant with given category and name. Returns true if found
     * and sets the var.
     * @param category to look for.
     * @param name of the variant to look for.
     * @param output variant.
     * @returns true if the variant found.
     */
    bool LookUp(StringView category, StringView name, ParameterVariant** var);

    /**
     * Can be used to expose or hide variants in the block by category.
     * @param exposed States if the parameter will be exposed or hidden.
     * @param category The category to set exposed status.
     */
    void ExposeByCategory(bool exposed, const VariantCategory& category);

   protected:
    /**
     * Serializes the ParameterBlock to the xml document.
     * @param doc The xml document object to serialize to.
     * @param parent The parent xml node to serialize to.
     */
    XmlNode* SerializeImp(XmlDocument* doc, XmlNode* parent) const override;

    /**
     * De serializes the ParameterBlcok from the xml document.
     * @param doc The xml document object to read from.
     * @param parent The parent xml node to read from.
     */
    XmlNode* DeSerializeImp(const SerializationFileInfo& info, XmlNode* parent) override;

   public:
    /**
     * Container vector for ParameterVariants.
     */
    ParameterVariantArray m_variants;
  };

  /**
   * Returns the selected value of multichoice variant.
   * Function is located here because it requires ParameterVariant to be declared before itself.
   */
  template <typename T>
  const T& MultiChoiceVariant::GetValue() const
  {
    return Choices[CurrentVal.Index].GetCVar<T>();
  }

  /**
   * Return an enum from multichoice variant.
   * Function is located here because it requires ParameterVariant to be declared before itself.
   */
  template <typename EnumT>
  const EnumT MultiChoiceVariant::GetEnum() const
  {
    return (EnumT) Choices[CurrentVal.Index].GetCVar<int>();
  }

  /** Helper functrion to create a multichoice parameter. */
  template <typename T>
  ParameterVariant CreateMultiChoiceParameter(const String& name, const T& val)
  {
    ParameterVariant param {val};
    param.m_name = name;
    return param;
  }

} // namespace ToolKit
