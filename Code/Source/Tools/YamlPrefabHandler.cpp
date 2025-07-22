
#include "YamlPrefabHandler.h"

#include <AzCore/Interface/Interface.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>

#include <fkYAML/node.hpp>

#include <cctype>
#include <sstream>
#include <string>

namespace JHO3DETestGem
{
    namespace
    {
        using AzToolsFramework::Prefab::PrefabDom;
        using AzToolsFramework::Prefab::PrefabDomValue;

        PrefabDomValue YamlToDom(const fkyaml::node& node, PrefabDom::AllocatorType& allocator)
        {
            if (node.is_null())
            {
                return PrefabDomValue(rapidjson::kNullType);
            }
            if (node.is_boolean())
            {
                return PrefabDomValue(node.get_value<bool>());
            }
            if (node.is_integer())
            {
                // Component and entity IDs are uint64_t, so a non-negative integer reads as unsigned to keep
                // values above INT64_MAX intact.
                if (node.is_uint())
                {
                    return PrefabDomValue(node.get_value<uint64_t>());
                }
                return PrefabDomValue(node.get_value<int64_t>());
            }
            if (node.is_float_number())
            {
                return PrefabDomValue(node.get_value<double>());
            }
            if (node.is_string())
            {
                const std::string& text = node.as_str();
                return PrefabDomValue(text.c_str(), static_cast<rapidjson::SizeType>(text.size()), allocator);
            }
            if (node.is_sequence())
            {
                PrefabDomValue array(rapidjson::kArrayType);
                for (const auto& item : node)
                {
                    array.PushBack(YamlToDom(item, allocator), allocator);
                }
                return array;
            }

            PrefabDomValue object(rapidjson::kObjectType);
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                if (!it.key().is_string())
                {
                    throw std::runtime_error("mapping keys must be strings");
                }
                const std::string& key = it.key().as_str();
                object.AddMember(
                    PrefabDomValue(key.c_str(), static_cast<rapidjson::SizeType>(key.size()), allocator),
                    YamlToDom(*it, allocator),
                    allocator);
            }
            return object;
        }

        bool EqualsIgnoreCase(AZStd::string_view text, AZStd::string_view word)
        {
            if (text.size() != word.size())
            {
                return false;
            }
            for (size_t i = 0; i < text.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(text[i])) != word[i])
                {
                    return false;
                }
            }
            return true;
        }

        //! Checks if a string reads back as the same string when written without quotes. Errs towards quoting, which
        //! is always correct.
        bool CanBePlain(AZStd::string_view text)
        {
            if (text.empty() || text.front() == ' ' || text.back() == ' ' || text.back() == ':')
            {
                return false;
            }

            // Indicators that start another kind of node, and characters that start a number.
            constexpr AZStd::string_view LeadingIndicators = "-?:,[]{}#&*!|>'\"%@`<=.+~";
            if (LeadingIndicators.find(text.front()) != AZStd::string_view::npos || std::isdigit(static_cast<unsigned char>(text.front())))
            {
                return false;
            }

            // Words that YAML 1.1 or 1.2 reads as null or a boolean.
            for (AZStd::string_view word : { "null", "true", "false", "yes", "no", "on", "off", "y", "n" })
            {
                if (EqualsIgnoreCase(text, word))
                {
                    return false;
                }
            }

            for (const char c : text)
            {
                if (static_cast<unsigned char>(c) < 0x20 || c == '\x7f')
                {
                    return false;
                }
            }
            return text.find(": ") == AZStd::string_view::npos && text.find(" #") == AZStd::string_view::npos;
        }

        //! Writes a value as JSON, whose double-quoted strings and numbers are valid YAML.
        void WriteAsJson(const PrefabDomValue& value, AZStd::string& out)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            value.Accept(writer);
            out.append(buffer.GetString(), buffer.GetSize());
        }

        void WriteString(const PrefabDomValue& value, AZStd::string& out)
        {
            const AZStd::string_view text(value.GetString(), value.GetStringLength());
            if (CanBePlain(text))
            {
                out += text;
            }
            else
            {
                WriteAsJson(value, out);
            }
        }

        bool IsBlock(const PrefabDomValue& value)
        {
            return (value.IsObject() && value.MemberCount() > 0) || (value.IsArray() && !value.Empty());
        }

        void WriteBlock(const PrefabDomValue& value, size_t indent, bool continuesLine, AZStd::string& out);

        //! Writes the value that follows a "key:" or "-" indicator.
        void WriteNodeValue(const PrefabDomValue& value, size_t indent, AZStd::string& out)
        {
            if (IsBlock(value))
            {
                out += '\n';
                WriteBlock(value, indent, false, out);
                return;
            }

            out += ' ';
            if (value.IsString())
            {
                WriteString(value, out);
            }
            else if (value.IsObject())
            {
                out += "{}";
            }
            else if (value.IsArray())
            {
                out += "[]";
            }
            else
            {
                WriteAsJson(value, out);
            }
            out += '\n';
        }

        //! Writes a non-empty mapping or sequence, one entry per line. When continuesLine is set, the first entry
        //! continues the current line, as in "- key: value".
        void WriteBlock(const PrefabDomValue& value, size_t indent, bool continuesLine, AZStd::string& out)
        {
            if (value.IsObject())
            {
                for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
                {
                    if (!continuesLine)
                    {
                        out.append(indent, ' ');
                    }
                    continuesLine = false;
                    WriteString(member->name, out);
                    out += ':';
                    WriteNodeValue(member->value, indent + 2, out);
                }
                return;
            }

            for (const PrefabDomValue& item : value.GetArray())
            {
                if (!continuesLine)
                {
                    out.append(indent, ' ');
                }
                continuesLine = false;
                out += '-';
                if (item.IsObject() && item.MemberCount() > 0)
                {
                    out += ' ';
                    WriteBlock(item, indent + 2, true, out);
                }
                else
                {
                    WriteNodeValue(item, indent + 2, out);
                }
            }
        }

        AZ::Outcome<PrefabDom, AZStd::string> LoadYamlPrefab(AZStd::string_view fileContent, AZ::IO::PathView filePath)
        {
            PrefabDom dom;
            try
            {
                std::istringstream stream(std::string(fileContent.data(), fileContent.size()));
                const fkyaml::node root = fkyaml::node::deserialize(stream);
                static_cast<PrefabDomValue&>(dom) = YamlToDom(root, dom.GetAllocator());
            } catch (const std::exception& e)
            {
                return AZ::Failure(AZStd::string::format("Failed to read '%.*s': %s", AZ_STRING_ARG(filePath.Native()), e.what()));
            }
            return AZ::Success(AZStd::move(dom));
        }

        //! Writes a Prefab DOM as YAML. Unsigned 64-bit values stay plain integers, and strings that YAML would read
        //! as another type, or reject, are double-quoted.
        AZ::Outcome<void, AZStd::string> SaveYamlPrefab(const PrefabDom& dom, AZ::IO::PathView absolutePath)
        {
            AZStd::string yaml;
            if (IsBlock(dom))
            {
                WriteBlock(dom, 0, false, yaml);
            }
            else
            {
                WriteNodeValue(dom, 0, yaml);
                yaml.erase(0, 1);
            }
            return AZ::Utils::WriteFile(yaml, absolutePath.Native());
        }
    } // anonymous namespace

    void RegisterYamlPrefabHandler()
    {
        auto* loader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        if (!loader)
        {
            AZ_Warning("YamlPrefabHandler", false, "PrefabLoaderInterface is not available, so '.yaml' files are not handled.");
            return;
        }
        loader->RegisterTemplateFileHandler(".yaml", &LoadYamlPrefab, &SaveYamlPrefab);
        AZ_TracePrintf("YamlPrefabHandler", "Handling '.yaml' files as Prefab Templates.\n");
    }

    void UnregisterYamlPrefabHandler()
    {
        auto* loader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        if (loader)
        {
            loader->UnregisterTemplateFileHandler(".yaml");
        }
    }
} // namespace JHO3DETestGem
