#include <iostream>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>

#include <map>

#include "ChunkArray.h"

 
//  µÃÂ¿‡
class Entity {
public:
    Entity(int id) : id(id) {}

    template<typename T>
    void addComponent(std::shared_ptr<T> component) {
        components[typeid(T)] = component;
    }

    template<typename T>
    T* getComponent() {
        auto it = components.find(typeid(T));
        if (it != components.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    template<typename T>
    const T* getComponent() const {
        auto it = components.find(typeid(T));
        if (it != components.end()) {
            return static_cast<const T*>(it->second.get());
        }
        return nullptr;
    }

    int getId() const { return id; }

private:
    int id;
    std::unordered_map<std::type_index, std::shared_ptr<Component>> components;
};
 