#pragma once
#include "../common/Common.h"

namespace DistributedChat {
    class ConfigManager {
    private:
        std::unordered_map<std::string, std::string> m_configs;

    public:
        ConfigManager() = default;
        bool Load(const std::string& filepath);
        std::string GetString(const std::string& key, const std::string& defaultVal = "") const;
        int GetInt(const std::string& key, int defaultVal = 0) const;
        bool GetBool(const std::string& key, bool defaultVal = false) const;
    };
}