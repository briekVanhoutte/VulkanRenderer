#pragma once
#include <string>
#include <fstream>
#include <optional>
#include <nlohmann/json.hpp>
#include "Singleton.h"   // your template from the prompt
#include <unordered_set>
#include <vector>

class Settings : public Singleton<Settings> {
    friend class Singleton<Settings>;
public:
    using json = nlohmann::json;

    // Initialize from a file; loads if it exists, otherwise starts empty.
    // You can call this once early (e.g., in Game::init()).
    void Initialize(const std::string& path = "settings.json");

    // Save now (also happens automatically in ~Settings()).
    void Save() const;

    // Merge a JSON of defaults (non-destructively).
    // Existing keys win; missing keys are filled from 'defaults'.
    void MergeDefaults(const json& defaults);

    // Remove a key (dot path). Returns true if it existed.
    bool Erase(const std::string& path);

    // Generic setter/getter with defaults. Supports dot-path, creates parents.
    template<typename T>
    void Set(const std::string& path, const T& value) {
        json* j = getPtrCreate(path);
        *j = value;
        m_Dirty = true;
    }

    template<typename T>
    T Get(const std::string& path, const T& fallback) const {
        const json* j = getPtr(path);
        if (!j || j->is_null()) return fallback;
        try {
            if constexpr (std::is_same_v<T, float>) {
                // json numbers are double by default; cast safely
                return j->is_number() ? static_cast<float>(j->get<double>()) : fallback;
            }
            else {
                return j->get<T>();
            }
        }
        catch (...) { return fallback; }
    }

    bool Has(const std::string& path) const { return getPtr(path) != nullptr; }
    const json& Data() const { return m_Data; }  // read-only snapshot
    json& Data() { m_Dirty = true; return m_Data; } // direct edit (advanced)

    ~Settings();  // auto-saves if dirty

    uint64_t Version() const { return m_Version; }
    std::vector<std::string> ConsumeChangedKeys(); // returns and clears changed keys
    bool AnyChanged() const { return !m_Changed.empty(); }

private:
    Settings() = default;

    // Dot-path helpers (create = true to create missing objects)
    nlohmann::json* getPtrCreate(const std::string& path);
    const nlohmann::json* getPtr(const std::string& path) const;

    static void           ensureObject(nlohmann::json& j);
    static std::vector<std::string> splitPath(const std::string& path);


    uint64_t m_Version = 0;
    std::unordered_set<std::string> m_Changed;

    void markChanged(const std::string& path) { ++m_Version; m_Dirty = true; m_Changed.insert(path); }

private:
    json        m_Data;
    std::string m_Path = "settings.json";
    mutable bool m_Dirty = false;
};
