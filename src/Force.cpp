#include "Force.hpp"
#include "Cloth.hpp"

glm::vec3 GravityForce::calculate(const Mass &mass, float time) const
{
    if (!enabled)
        return glm::vec3(0.0f);
    return glm::vec3(0.0f, gravity * mass.mass, 0.0f);
}

glm::vec3 ForceManager::calculateTotalForce(const Mass &mass, float time) const
{
    glm::vec3 total(0.0f);
    for (const auto &force : forces)
        total += force->calculate(mass, time);
    return total;
}

WindForce::WindForce(const glm::vec3 &dir, float str) : direction(glm::normalize(dir)), strength(str)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    this->enabled = false;
}

glm::vec3 WindForce::calculate(const Mass &mass, float time) const
{
    if (!enabled)
        return glm::vec3(0.0f);
    return direction * strength;
}

glm::vec3 WindForce::getDirection() const
{
    return direction;
}
void WindForce::setDirection(const glm::vec3 &dir)
{
    if (glm::length(dir) > 0.001f)
        direction = glm::normalize(dir);
}
float WindForce::getStrength() const
{
    return strength;
}
void WindForce::setStrength(float str)
{
    distr = std::uniform_int_distribution<>(strength / 2, strength);
    strength = str;
}

GravityForce::GravityForce(float g = -9.81f) : gravity(g)
{
}

float GravityForce::getGravity() const
{
    return gravity;
}
void GravityForce::setGravity(float g)
{
    gravity = g;
}

OscillatingForce::OscillatingForce(const glm::vec3 &dir, float amp, float freq)
    : direction(glm::normalize(dir)), amplitude(amp), frequency(freq)
{
}

glm::vec3 OscillatingForce::calculate(const Mass &mass, float time) const
{
    if (!enabled)
        return glm::vec3(0.0f);

    float wave = std::sin(time * frequency) * amplitude;
    return direction * wave;
}

glm::vec3 OscillatingForce::getDirection() const
{
    return direction;
}
void OscillatingForce::setDirection(const glm::vec3 &dir)
{
    if (glm::length(dir) > 0.001f)
        direction = glm::normalize(dir);
}

float OscillatingForce::getAmplitude() const
{
    return amplitude;
}
void OscillatingForce::setAmplitude(float amp)
{
    amplitude = amp;
}

float OscillatingForce::getFrequency() const
{
    return frequency;
}
void OscillatingForce::setFrequency(float freq)
{
    frequency = freq;
}