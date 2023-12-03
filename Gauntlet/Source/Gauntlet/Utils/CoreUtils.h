#pragma once

#include <Gauntlet/Core/Log.h>
#include <vector>
#include <fstream>

namespace Gauntlet
{

namespace Utility
{
static std::vector<uint8_t> LoadDataFromDisk(const std::string& filePath)
{
    std::vector<uint8_t> data;

    std::ifstream in(filePath.data(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!in.is_open())
    {
        LOG_WARN("Failed to load %s!", filePath.data());
        return data;
    }

    data.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(data.data()), data.size());

    in.close();
    return data;
}

static bool SaveDataToDisk(const void* data, const size_t dataSize, const std::string& filePath)
{
    std::ofstream out(filePath.data(), std::ios::out | std::ios::binary);
    if (!out.is_open())
    {
        LOG_WARN("Failed to load %s!", filePath.data());
        return false;
    }

    out.write((char*)data, dataSize);

    out.close();
    return true;
}

}  // namespace Utility
}  // namespace Gauntlet