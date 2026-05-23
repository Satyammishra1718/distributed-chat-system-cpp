#include "../../../include/core/config/ConfigManager.h"

namespace DistributedChat {
    bool ConfigManager::Load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) {
                continue;
            }

            std::string key = Trim(line.substr(0, eqPos));
            std::string val = Trim(line.substr(eqPos + 1));
            if (!key.empty()) {
                m_configs[key] = val;
            }
        }
        return true;
    }

    std::string ConfigManager::GetString(const std::string& key, const std::string& defaultVal) const {
        auto it = m_configs.find(key);
        if (it != m_configs.end()) {
            return it->second;
        }
        return defaultVal;
    }

    int ConfigManager::GetInt(const std::string& key, int defaultVal) const {
        auto it = m_configs.find(key);
        if (it != m_configs.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {}
        }
        return defaultVal;
    }

    bool ConfigManager::GetBool(const std::string& key, bool defaultVal) const {
        auto it = m_configs.find(key);
        if (it != m_configs.end()) {
            std::string val = it->second;
            std::transform(val.begin(), val.end(), val.begin(), ::tolower);
            if (val == "true" || val == "yes" || val == "1") return true;
            if (val == "false" || val == "no" || val == "0") return false;
        }
        return defaultVal;
    }
}