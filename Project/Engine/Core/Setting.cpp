#include "Settings.h"
#include <sstream>

using json = nlohmann::json;

static bool file_exists(const std::string& p) {
    std::ifstream f(p, std::ios::in | std::ios::binary);
    return f.good();
}

void Settings::Initialize(const std::string& path) {
    m_Path = path.empty() ? std::string("settings.json") : path;
    if (file_exists(m_Path)) {
        std::ifstream in(m_Path);
        try { in >> m_Data; }
        catch (...) { m_Data = json::object(); }
    }
    else {
        m_Data = json::object();
    }
    m_Dirty = false;
}

void Settings::Save() const {
    try {
        std::ofstream out(m_Path, std::ios::out | std::ios::trunc);
        out << m_Data.dump(2);
        out.flush();
        m_Dirty = false;
    }
    catch (...) {
        // swallow; keep dirty so we try again later if desired
    }
}

void Settings::MergeDefaults(const json& defaults) {
    // non-destructive merge: only fill missing keys
    std::function<void(json&, const json&)> merge = [&](json& dst, const json& def) {
        if (!def.is_object()) return;
        ensureObject(dst);
        for (auto it = def.begin(); it != def.end(); ++it) {
            const std::string key = it.key();
            if (!dst.contains(key)) {
                dst[key] = it.value();
                m_Dirty = true;
            }
            else if (it->is_object()) {
                merge(dst[key], *it);
            }
        }
        };
    merge(m_Data, defaults);
}

bool Settings::Erase(const std::string& path) {
    auto toks = splitPath(path);
    if (toks.empty()) return false;
    json* cur = &m_Data;
    for (size_t i = 0; i + 1 < toks.size(); ++i) {
        if (!cur->contains(toks[i]) || !(*cur)[toks[i]].is_object()) return false;
        cur = &(*cur)[toks[i]];
    }
    bool existed = cur->contains(toks.back());
    if (existed) {
        cur->erase(toks.back());
        m_Dirty = true;
    }
    return existed;
}

Settings::~Settings() {
    if (m_Dirty) Save();
}

std::vector<std::string> Settings::ConsumeChangedKeys() {
    std::vector<std::string> out; out.reserve(m_Changed.size());
    for (auto& k : m_Changed) out.push_back(k);
    m_Changed.clear();
    return out;
}

std::vector<std::string> Settings::splitPath(const std::string& path) {
    std::vector<std::string> out;
    std::stringstream ss(path);
    std::string tok;
    while (std::getline(ss, tok, '.')) {
        if (!tok.empty()) out.push_back(tok);
    }
    return out;
}

void Settings::ensureObject(json& j) {
    if (!j.is_object()) j = json::object();
}

json* Settings::getPtrCreate(const std::string& path) {
    auto toks = splitPath(path);
    if (toks.empty()) return &m_Data;
    json* cur = &m_Data;
    for (size_t i = 0; i + 1 < toks.size(); ++i) {
        ensureObject(*cur);
        cur = &(*cur)[toks[i]];
    }
    ensureObject(*cur);
    return &(*cur)[toks.back()];
}

const json* Settings::getPtr(const std::string& path) const {
    auto toks = splitPath(path);
    if (toks.empty()) return &m_Data;
    const json* cur = &m_Data;
    for (size_t i = 0; i + 1 < toks.size(); ++i) {
        auto it = cur->find(toks[i]);
        if (it == cur->end() || !it->is_object()) return nullptr;
        cur = &(*it);
    }
    auto leaf = cur->find(toks.back());
    if (leaf == cur->end()) return nullptr;
    return &(*leaf);
}
