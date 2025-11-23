#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <cmath>

struct Mass;

class Force
{
public:
    virtual ~Force() = default;
    virtual glm::vec3 calculate(const Mass& mass, float time) const = 0;
    
    bool isEnabled() const { return enabled; }
    void setEnabled(bool state) { enabled = state; }
    
protected:
    bool enabled = true;
};

class GravityForce : public Force
{
public:
    GravityForce(float g = -9.81f) : gravity(g) {}
    
    glm::vec3 calculate(const Mass& mass, float time) const override;
    
    float getGravity() const { return gravity; }
    void setGravity(float g) { gravity = g; }
    
private:
    float gravity;
};

class WindForce : public Force
{
public:
    WindForce(const glm::vec3& dir, float str) 
        : direction(glm::normalize(dir)), strength(str) 
        {
            this->enabled = false;
        }
    
    glm::vec3 calculate(const Mass& mass, float time) const override
    {
        if (!enabled) return glm::vec3(0.0f);
        return direction * strength;
    }
    
    glm::vec3 getDirection() const { return direction; }
    void setDirection(const glm::vec3& dir) 
    { 
        if (glm::length(dir) > 0.001f)
            direction = glm::normalize(dir); 
    }
    
    float getStrength() const { return strength; }
    void setStrength(float str) { strength = str; }
    
private:
    glm::vec3 direction;
    float strength;
};

class OscillatingForce : public Force
{
public:
    OscillatingForce(const glm::vec3& dir, float amp, float freq)
        : direction(glm::normalize(dir)), amplitude(amp), frequency(freq) {}
    
    glm::vec3 calculate(const Mass& mass, float time) const override
    {
        if (!enabled) return glm::vec3(0.0f);
        
        float wave = std::sin(time * frequency) * amplitude;
        return direction * wave;
    }
    
    glm::vec3 getDirection() const { return direction; }
    void setDirection(const glm::vec3& dir) 
    { 
        if (glm::length(dir) > 0.001f)
            direction = glm::normalize(dir); 
    }
    
    float getAmplitude() const { return amplitude; }
    void setAmplitude(float amp) { amplitude = amp; }
    
    float getFrequency() const { return frequency; }
    void setFrequency(float freq) { frequency = freq; }
    
private:
    glm::vec3 direction;
    float amplitude;
    float frequency;
};

class ForceManager
{
public:
    template<typename T, typename... Args>
    T* addForce(Args&&... args)
    {
        auto force = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = force.get();
        forces.push_back(std::move(force));
        
        std::type_index typeIdx = typeid(T);
        forcesByType[typeIdx].push_back(ptr);
        
        return ptr;
    }
    
    template<typename T>
    T* getForce()
    {
        std::type_index typeIdx = typeid(T);
        auto it = forcesByType.find(typeIdx);
        if (it != forcesByType.end() && !it->second.empty())
            return static_cast<T*>(it->second[0]);
        return nullptr;
    }
    
    template<typename T>
    std::vector<T*> getForces()
    {
        std::type_index typeIdx = typeid(T);
        auto it = forcesByType.find(typeIdx);
        if (it != forcesByType.end())
        {
            std::vector<T*> result;
            for (Force* f : it->second)
                result.push_back(static_cast<T*>(f));
            return result;
        }
        return {};
    }
    
    glm::vec3 calculateTotalForce(const Mass& mass, float time) const;
    
    void clear()
    {
        forces.clear();
        forcesByType.clear();
    }
    
    void update(float dt) { }
    
private:
    std::vector<std::unique_ptr<Force>> forces;
    std::unordered_map<std::type_index, std::vector<Force*>> forcesByType;
};
