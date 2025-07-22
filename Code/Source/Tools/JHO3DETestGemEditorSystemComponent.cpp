
#include "JHO3DETestGemEditorSystemComponent.h"
#include "YamlPrefabHandler.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <JHO3DETestGem/JHO3DETestGemTypeIds.h>

namespace JHO3DETestGem
{
    AZ_COMPONENT_IMPL(JHO3DETestGemEditorSystemComponent, "JHO3DETestGemEditorSystemComponent", JHO3DETestGemEditorSystemComponentTypeId);

    void JHO3DETestGemEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JHO3DETestGemEditorSystemComponent, AZ::Component>()->Version(0);
        }
    }

    void JHO3DETestGemEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("JHO3DETestGemEditorService"));
    }

    void JHO3DETestGemEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("JHO3DETestGemEditorService"));
    }

    JHO3DETestGemEditorSystemComponent::JHO3DETestGemEditorSystemComponent() = default;

    JHO3DETestGemEditorSystemComponent::~JHO3DETestGemEditorSystemComponent() = default;

    void JHO3DETestGemEditorSystemComponent::Activate()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        RegisterYamlPrefabHandler();
    }

    void JHO3DETestGemEditorSystemComponent::Deactivate()
    {
        UnregisterYamlPrefabHandler();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails JHO3DETestGemEditorSystemComponent::GetSourceFileDetails(
        const char* fullSourceFileName)
    {
        AZStd::string extension;
        AZ::StringFunc::Path::GetExtension(fullSourceFileName, extension);
        if (AZ::StringFunc::Equal(extension.c_str(), ".yaml"))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("Icons/AssetBrowser/Prefab_80.svg");
        }
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }
} // namespace JHO3DETestGem
