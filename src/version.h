#pragma once

#include <string>
#include <string_view>
#include <sstream>

class sem_version
{
public:
    int major;
    int minor;
    int patch;
    constexpr sem_version() noexcept : major(0), minor(0), patch(0) { }
    constexpr sem_version(int major, int minor, int patch) noexcept : major(major), minor(minor), patch(patch) { }
};

class app_version
{
public:
    std::string name;
    sem_version s_version;
    std::string version;
    std::string license;
    std::string description;
    std::string homepage;

    [[nodiscard]] static app_version const &data() noexcept;
    [[nodiscard]] static sem_version const &get_semantic_version() noexcept;
    [[nodiscard]] static std::string const &get_version() noexcept;
};
