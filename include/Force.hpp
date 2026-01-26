#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <memory>
#include <random>
#include <typeindex>
#include <unordered_map>
#include <vector>

struct Mass;

// Base Force class
class Force
{
  public:
    virtual ~Force() = default;
    // Calculate mass force
    virtual glm::vec3 calculate(const Mass &mass, float time) const = 0;
    // Enable/disable force
    bool isEnabled() const
    {
        return enabled;
    }
    void setEnabled(bool state)
    {
        enabled = state;
    }

  protected:
    bool enabled = true;
};

class GravityForce : public Force
{
  public:
    GravityForce(float g);

    glm::vec3 calculate(const Mass &mass, float time) const override;

    float getGravity() const;
    void setGravity(float g);

  private:
    float gravity;
};

class WindForce : public Force
{
  public:
    WindForce(const glm::vec3 &dir, float str);

    glm::vec3 calculate(const Mass &mass, float time) const override;

    // Get wind direction and strength
    glm::vec3 getDirection() const;
    void setDirection(const glm::vec3 &dir);
    float getStrength() const;
    void setStrength(float str);

  private:
    glm::vec3 direction;
    float strength;
    std::uniform_int_distribution<> distr;
};

class OscillatingForce : public Force
{
  public:
    OscillatingForce(const glm::vec3 &dir, float amp, float freq);

    glm::vec3 calculate(const Mass &mass, float time) const override;

    // Direction
    glm::vec3 getDirection() const;
    void setDirection(const glm::vec3 &dir);
    // Wave Size
    float getAmplitude() const;
    void setAmplitude(float amp);
    // Wave Size
    float getFrequency() const;
    void setFrequency(float freq);

  private:
    glm::vec3 direction;
    float amplitude;
    float frequency;
};

class ForceManager
{
  public:
    template <typename T, typename... Args> T *addForce(Args &&...args)
    {
        auto force = std::make_unique<T>(std::forward<Args>(args)...);
        T *ptr = force.get();
        forces.push_back(std::move(force));

        std::type_index typeIdx = typeid(T);
        forcesByType[typeIdx].push_back(ptr);

        return ptr;
    }

    template <typename T> T *getForce()
    {
        std::type_index typeIdx = typeid(T);
        auto it = forcesByType.find(typeIdx);
        if (it != forcesByType.end() && !it->second.empty())
            return static_cast<T *>(it->second[0]);
        return nullptr;
    }

    template <typename T> std::vector<T *> getForces()
    {
        std::type_index typeIdx = typeid(T);
        auto it = forcesByType.find(typeIdx);
        if (it != forcesByType.end())
        {
            std::vector<T *> result;
            for (Force *f : it->second)
                result.push_back(static_cast<T *>(f));
            return result;
        }
        return {};
    }

    glm::vec3 calculateTotalForce(const Mass &mass, float time) const;

    void clear()
    {
        forces.clear();
        forcesByType.clear();
    }

    void update(float dt)
    {
    }

  private:
    std::vector<std::unique_ptr<Force>> forces;
    std::unordered_map<std::type_index, std::vector<Force *>> forcesByType;
};
