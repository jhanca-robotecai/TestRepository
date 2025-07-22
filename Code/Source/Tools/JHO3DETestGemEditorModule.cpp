
#include <AzCore/Module/Module.h>
#include <JHO3DETestGem/JHO3DETestGemTypeIds.h>
#include <Tools/JHO3DETestGemEditorSystemComponent.h>

namespace JHO3DETestGem
{
    class JHO3DETestGemEditorModule : public AZ::Module
    {
    public:
        AZ_RTTI(JHO3DETestGemEditorModule, JHO3DETestGemEditorModuleTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(JHO3DETestGemEditorModule, AZ::SystemAllocator);

        JHO3DETestGemEditorModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    JHO3DETestGemEditorSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<JHO3DETestGemEditorSystemComponent>(),
            };
        }
    };
} // namespace JHO3DETestGem

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), JHO3DETestGem::JHO3DETestGemEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_JHO3DETestGem_Editor, JHO3DETestGem::JHO3DETestGemEditorModule)
#endif
