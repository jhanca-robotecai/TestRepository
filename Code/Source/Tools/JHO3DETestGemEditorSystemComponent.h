
#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

namespace JHO3DETestGem
{
    //! Registers '.yaml' as a Prefab Template file format, so the Editor loads and saves YAML wherever it handles
    //! '.prefab', and gives '.yaml' files the Prefab icon in the Asset Browser.
    class JHO3DETestGemEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(JHO3DETestGemEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        JHO3DETestGemEditorSystemComponent();
        ~JHO3DETestGemEditorSystemComponent();

    private:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AssetBrowserInteractionNotificationBus
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
    };
} // namespace JHO3DETestGem
