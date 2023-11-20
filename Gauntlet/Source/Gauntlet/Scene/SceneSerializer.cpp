#include "GauntletPCH.h"
#include "SceneSerializer.h"

#include "Gauntlet/Core/Application.h"
#include "Gauntlet/Core/JobSystem.h"

#include "Scene.h"
#include "Entity.h"
#include "Components.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace Gauntlet
{

static void SerializeEntity(nlohmann::ordered_json& out, Entity entity)
{
    GNT_ASSERT(entity.HasComponent<IDComponent>(), "Every entity should have ID!");

    const std::string UUIDstring = std::to_string(entity.GetUUID());
    out.emplace(UUIDstring, out.object());
    auto& node = out[UUIDstring];

    // Each entity has it whatever happens.
    if (entity.HasComponent<TagComponent>())
    {
        auto& tc = entity.GetComponent<TagComponent>();
        node.emplace("TagComponent", tc.Tag);
    }

    if (entity.HasComponent<TransformComponent>())
    {
        auto& tc = entity.GetComponent<TransformComponent>();
        node["TransformComponent"].emplace("Translation",
                                           std::initializer_list<float>({tc.Translation.x, tc.Translation.y, tc.Translation.z}));
        node["TransformComponent"].emplace("Rotation", std::initializer_list<float>({tc.Rotation.x, tc.Rotation.y, tc.Rotation.z}));
        node["TransformComponent"].emplace("Scale", std::initializer_list<float>({tc.Scale.x, tc.Scale.y, tc.Scale.z}));
    }

    if (entity.HasComponent<SpriteRendererComponent>())
    {
        auto& src = entity.GetComponent<SpriteRendererComponent>();
        node["SpriteRendererComponent"].emplace("Color",
                                                std::initializer_list<float>({src.Color.x, src.Color.y, src.Color.z, src.Color.w}));
    }

    if (entity.HasComponent<CameraComponent>())
    {
        auto& cc = entity.GetComponent<CameraComponent>();
        node["CameraComponent"].emplace("Active", cc.bIsActive ? "true" : "false");
    }

    if (entity.HasComponent<MeshComponent>())
    {
        auto& mc = entity.GetComponent<MeshComponent>();
        node["MeshComponent"].emplace("Name", mc.Mesh->GetMeshNameWithDirectory());
    }

    if (entity.HasComponent<PointLightComponent>())
    {
        auto& plc = entity.GetComponent<PointLightComponent>();
        node["PointLightComponent"].emplace("Color", std::initializer_list<float>({plc.Color.x, plc.Color.y, plc.Color.z}));
        node["PointLightComponent"].emplace(
            "AmbientSpecularShininess",
            std::initializer_list<float>({plc.AmbientSpecularShininess.x, plc.AmbientSpecularShininess.y, plc.AmbientSpecularShininess.z}));
        node["PointLightComponent"].emplace("Active", std::initializer_list<bool>({plc.bIsActive}));
    }

    if (entity.HasComponent<DirectionalLightComponent>())
    {
        auto& dlc = entity.GetComponent<DirectionalLightComponent>();
        node["DirectionalLightComponent"].emplace("Color", std::initializer_list<float>({dlc.Color.x, dlc.Color.y, dlc.Color.z}));
        node["DirectionalLightComponent"].emplace("CastShadows", std::initializer_list<bool>({dlc.bCastShadows}));
    }

    if (entity.HasComponent<SpotLightComponent>())
    {
        auto& slc = entity.GetComponent<SpotLightComponent>();
        node["SpotLightComponent"].emplace("Color", std::initializer_list<float>({slc.Color.x, slc.Color.y, slc.Color.z}));
        node["SpotLightComponent"].emplace(
            "AmbientSpecularShininess",
            std::initializer_list<float>({slc.AmbientSpecularShininess.x, slc.AmbientSpecularShininess.y, slc.AmbientSpecularShininess.z}));
        node["SpotLightComponent"].emplace("CutOff", std::initializer_list<float>({slc.CutOff}));
        node["SpotLightComponent"].emplace("Active", std::initializer_list<bool>({slc.bIsActive}));
    }
}

SceneSerializer::SceneSerializer(Ref<Scene>& scene) : m_Scene(scene) {}

void SceneSerializer::Serialize(const std::string& filePath)
{
    const auto serializeBegin = Application::Get().GetTimeNow();

    nlohmann::ordered_json json;
    json["Scene"] = m_Scene->GetName();

    json.emplace("Entities", json.object());
    auto& entities_node = json["Entities"];
    m_Scene->m_Registry.each(
        [&](auto entityID)
        {
            Entity entity(entityID, m_Scene.get());
            SerializeEntity(entities_node, entity);
        });

    std::ofstream out(filePath.data(), std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        LOG_WARN("Failed to serialize scene! %s", filePath.data());
        return;
    }

    out << std::setw(2) << json << std::endl;
    out.close();

    const auto serializeEnd = Application::Get().GetTimeNow();
    LOG_WARN("Time took to serialize \"%s\", (%0.2f) ms.", filePath.data(), (serializeEnd - serializeBegin) * 1000.0f);
}

void SceneSerializer::SerializeRuntime(const std::string& filePath)
{
    GNT_ASSERT(false, "Not implmented!");
}

bool SceneSerializer::Deserialize(const std::string& filePath)
{
    const auto deserializeBegin = Application::Get().GetTimeNow();

    std::ifstream in(filePath.data());
    if (!in.is_open())
    {
        LOG_WARN("Failed to open scene! %s", filePath.data());
        return false;
    }

    const nlohmann::ordered_json json = nlohmann::json::parse(in);
    in.close();

    std::string sceneName     = json["Scene"];
    const size_t slashPos     = filePath.find_last_of("/\\");
    const size_t extensionPos = filePath.find_last_of('.');

    if (sceneName == "Default" && slashPos != std::string::npos && extensionPos != std::string::npos)
        sceneName = std::string(filePath.begin() + slashPos + 1, filePath.begin() + extensionPos);

    m_Scene = MakeRef<Scene>(sceneName);

    // nlohmann::json::iterator
    // TODO: Move to static void DeserializeEntity()
    nlohmann::json entities = json["Entities"];
    for (auto& item : entities.items())
    {
        const nlohmann::json& node = item.value();

        const std::string tag = node["TagComponent"].get<std::string>();
        const uint64_t id     = std::stoull(item.key());
        Entity entity         = m_Scene->CreateEntityWithUUID(id, tag);

        {
            auto& tc                         = entity.GetComponent<TransformComponent>();
            std::array<float, 3> translation = node["TransformComponent"]["Translation"].get<std::array<float, 3>>();
            tc.Translation                   = glm::vec3(translation[0], translation[1], translation[2]);

            std::array<float, 3> rotation = node["TransformComponent"]["Rotation"].get<std::array<float, 3>>();
            tc.Rotation                   = glm::vec3(rotation[0], rotation[1], rotation[2]);

            std::array<float, 3> scale = node["TransformComponent"]["Scale"].get<std::array<float, 3>>();
            tc.Scale                   = glm::vec3(scale[0], scale[1], scale[2]);
        }

        if (node.contains("SpriteRendererComponent"))
        {
            auto& src                  = entity.AddComponent<SpriteRendererComponent>();
            std::array<float, 4> color = node["SpriteRendererComponent"]["Color"].get<std::array<float, 4>>();
            src.Color                  = glm::vec4(color[0], color[1], color[2], color[3]);
        }

        // TODO: Add camera component

        if (node.contains("MeshComponent"))
        {
            auto& mc                 = entity.AddComponent<MeshComponent>();
            std::string meshFilePath = node["MeshComponent"]["Name"].get<std::string>();
            mc.Mesh                  = Mesh::Create("Resources/Models/" + meshFilePath);
        }

        if (node.contains("PointLightComponent"))
        {
            auto& plc = entity.AddComponent<PointLightComponent>();

            std::array<float, 3> color = node["PointLightComponent"]["Color"].get<std::array<float, 3>>();
            plc.Color                  = glm::vec3(color[0], color[1], color[2]);

            std::array<float, 3> ambientSpecularShininess =
                node["PointLightComponent"]["AmbientSpecularShininess"].get<std::array<float, 3>>();
            plc.AmbientSpecularShininess = glm::vec3(ambientSpecularShininess[0], ambientSpecularShininess[1], ambientSpecularShininess[2]);

            std::array<bool, 1> active = node["PointLightComponent"]["Active"].get<std::array<bool, 1>>();
            plc.bIsActive              = active[0];
        }

        if (node.contains("DirectionalLightComponent"))
        {
            auto& dlc = entity.AddComponent<DirectionalLightComponent>();

            std::array<float, 3> color = node["DirectionalLightComponent"]["Color"].get<std::array<float, 3>>();
            dlc.Color                  = glm::vec3(color[0], color[1], color[2]);

            std::array<bool, 1> castShadows = node["DirectionalLightComponent"]["CastShadows"].get<std::array<bool, 1>>();
            dlc.bCastShadows                = castShadows[0];
        }

        if (node.contains("SpotLightComponent"))
        {
            auto& slc = entity.AddComponent<SpotLightComponent>();

            std::array<float, 3> color = node["SpotLightComponent"]["Color"].get<std::array<float, 3>>();
            slc.Color                  = glm::vec3(color[0], color[1], color[2]);

            std::array<float, 3> ambientSpecularShininess =
                node["SpotLightComponent"]["AmbientSpecularShininess"].get<std::array<float, 3>>();
            slc.AmbientSpecularShininess = glm::vec3(ambientSpecularShininess[0], ambientSpecularShininess[1], ambientSpecularShininess[2]);

            std::array<float, 1> cutOff = node["SpotLightComponent"]["CutOff"].get<std::array<float, 1>>();
            slc.CutOff                  = cutOff[0];

            std::array<bool, 1> active = node["SpotLightComponent"]["Active"].get<std::array<bool, 1>>();
            slc.bIsActive              = active[0];
        }
    }

    JobSystem::Wait();
    const auto deserializeEnd = Application::Get().GetTimeNow();
    LOG_WARN("Time took to deserialize \"%s\", (%0.2f) ms.", filePath.data(), (deserializeEnd - deserializeBegin) * 1000.0f);
    return true;
}

bool SceneSerializer::DeserializeRuntime(const std::string& filePath)
{
    GNT_ASSERT(false, "Not implmented!");
    return false;
}

}  // namespace Gauntlet