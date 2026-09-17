#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ImixMenuModel {

enum class ItemType : std::uint8_t { Toggle, Action, Value, Info };

struct Item {
    const char* id;
    const char* title;
    const char* description;
    ItemType type;
    const char* key = nullptr;
    int action = 0;
};

struct Category {
    const char* id;
    const char* title;
    const char* subtitle;
    std::vector<Item> items;
};

const std::vector<Category>& categories();
const Category* findCategory(int index);
int clampCategory(int index);

} // namespace ImixMenuModel
