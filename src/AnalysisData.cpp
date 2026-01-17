#include "AnalysisData.hpp"
#include "Cloth.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

ClothAnalysis::ClothAnalysis()
    : maxHistorySize(500), totalBrokenSprings(0),
      totalEnergy(0.0f), averageTension(0.0f), maxTension(0.0f),
      minBounds(0.0f), maxBounds(0.0f),
      isRecording(false), recordedMassIndex(-1)
{
    startTime = std::chrono::steady_clock::now();
}

void ClothAnalysis::update(float deltaTime, float simulationTime)
{
}

void ClothAnalysis::recordMassPointData(int massIndex, const Mass& mass, 
                                       const std::vector<Spring>& springs,
                                       float simulationTime)
{
    if (!isRecording) return;
    
    MassPointData data;
    data.time = simulationTime;
    data.position = mass.position;
    data.velocity = calculateVelocity(mass);
    data.acceleration = mass.acceleration;
    data.totalForce = mass.force;
    data.kineticEnergy = calculateKineticEnergy(mass);
    
    data.connectedSprings = 0;
    float totalTension = 0.0f;
    
    for (const auto& spring : springs)
    {
        if (spring.a == massIndex || spring.b == massIndex)
        {
            data.connectedSprings++;
            totalTension += 1.0f;
        }
    }
    
    if (data.connectedSprings > 0)
        data.averageSpringTension = totalTension / data.connectedSprings;
    
    historyData.push_back(data);
    
    while (historyData.size() > maxHistorySize)
    {
        historyData.pop_front();
    }
    
    recordedMassIndex = massIndex;
}

void ClothAnalysis::recordSpringBreak(int springIndex, const glm::vec3& position, 
                                     float tension, float simulationTime)
{
    breakEvents.emplace_back(simulationTime, springIndex, position, tension);
    totalBrokenSprings++;
    
    if (breakEvents.size() > 100)
    {
        breakEvents.erase(breakEvents.begin());
    }
}

float ClothAnalysis::calculateKineticEnergy(const Mass& mass) const
{
    glm::vec3 velocity = calculateVelocity(mass);
    float speed = glm::length(velocity);
    return 0.5f * mass.mass * speed * speed;
}

float ClothAnalysis::calculateAverageSpringTension(int massIndex, 
                                                   const std::vector<Mass>& masses,
                                                   const std::vector<Spring>& springs) const
{
    float totalTension = 0.0f;
    int count = 0;
    
    for (const auto& spring : springs)
    {
        if (spring.a == massIndex || spring.b == massIndex)
        {
            const Mass& massA = masses[spring.a];
            const Mass& massB = masses[spring.b];
            
            float currentLength = glm::length(massB.position - massA.position);
            float strain = (currentLength - spring.restLength) / spring.restLength;
            float tension = std::abs(strain);
            
            totalTension += tension;
            count++;
        }
    }
    
    return count > 0 ? totalTension / count : 0.0f;
}

glm::vec3 ClothAnalysis::calculateVelocity(const Mass& mass) const
{
    return mass.position - mass.prevPosition;
}

void ClothAnalysis::updateGlobalStats(const std::vector<Mass>& masses,
                                     const std::vector<Spring>& springs,
                                     float simulationTime)
{
    totalEnergy = 0.0f;
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& mass : masses)
    {
        totalEnergy += calculateKineticEnergy(mass);
        
        minBounds.x = std::min(minBounds.x, mass.position.x);
        minBounds.y = std::min(minBounds.y, mass.position.y);
        minBounds.z = std::min(minBounds.z, mass.position.z);
        
        maxBounds.x = std::max(maxBounds.x, mass.position.x);
        maxBounds.y = std::max(maxBounds.y, mass.position.y);
        maxBounds.z = std::max(maxBounds.z, mass.position.z);
    }
    
    float totalTension = 0.0f;
    maxTension = 0.0f;
    
    for (const auto& spring : springs)
    {
        if (spring.a >= masses.size() || spring.b >= masses.size())
            continue;
            
        const Mass& massA = masses[spring.a];
        const Mass& massB = masses[spring.b];
        
        float currentLength = glm::length(massB.position - massA.position);
        float strain = (currentLength - spring.restLength) / spring.restLength;
        float tension = std::abs(strain);
        
        totalTension += tension;
        maxTension = std::max(maxTension, tension);
    }
    
    averageTension = springs.empty() ? 0.0f : totalTension / springs.size();
}

void ClothAnalysis::clearHistory()
{
    historyData.clear();
    breakEvents.clear();
    totalBrokenSprings = 0;
}

std::string ClothAnalysis::exportToCSV() const
{
    std::stringstream ss;
    
    ss << "Time,PosX,PosY,PosZ,VelX,VelY,VelZ,Speed,";
    ss << "AccX,AccY,AccZ,KineticEnergy,ConnectedSprings,AvgTension\n";
    
    for (const auto& data : historyData)
    {
        float speed = glm::length(data.velocity);
        
        ss << std::fixed << std::setprecision(6);
        ss << data.time << ",";
        ss << data.position.x << "," << data.position.y << "," << data.position.z << ",";
        ss << data.velocity.x << "," << data.velocity.y << "," << data.velocity.z << ",";
        ss << speed << ",";
        ss << data.acceleration.x << "," << data.acceleration.y << "," << data.acceleration.z << ",";
        ss << data.kineticEnergy << ",";
        ss << data.connectedSprings << ",";
        ss << data.averageSpringTension << "\n";
    }
    
    return ss.str();
}