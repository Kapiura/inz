#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <string>
#include <chrono>

struct Mass;
struct Spring;

struct MassPointData
{
    float time;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 totalForce;
    float kineticEnergy;
    float averageSpringTension;
    int connectedSprings;
    
    MassPointData() 
        : time(0.0f), position(0.0f), velocity(0.0f), 
          acceleration(0.0f), totalForce(0.0f), 
          kineticEnergy(0.0f), averageSpringTension(0.0f),
          connectedSprings(0) {}
};

struct SpringBreakEvent
{
    float time;
    int springIndex;
    glm::vec3 position;
    float tension;
    
    SpringBreakEvent(float t, int idx, const glm::vec3& pos, float tens)
        : time(t), springIndex(idx), position(pos), tension(tens) {}
};

class ClothAnalysis
{
public:
    ClothAnalysis();
    
    void update(float deltaTime, float simulationTime);
    void recordMassPointData(int massIndex, const Mass& mass, 
                            const std::vector<Spring>& springs,
                            float simulationTime);
    void recordSpringBreak(int springIndex, const glm::vec3& position, 
                          float tension, float simulationTime);
    
    float calculateKineticEnergy(const Mass& mass) const;
    float calculateAverageSpringTension(int massIndex, 
                                       const std::vector<Mass>& masses,
                                       const std::vector<Spring>& springs) const;
    glm::vec3 calculateVelocity(const Mass& mass) const;
    
    void updateGlobalStats(const std::vector<Mass>& masses,
                          const std::vector<Spring>& springs,
                          float simulationTime);
    
    const std::deque<MassPointData>& getHistoryData() const { return historyData; }
    const std::vector<SpringBreakEvent>& getBreakEvents() const { return breakEvents; }
    
    float getTotalEnergy() const { return totalEnergy; }
    float getAverageTension() const { return averageTension; }
    int getTotalBrokenSprings() const { return totalBrokenSprings; }
    float getMaxTension() const { return maxTension; }
    glm::vec3 getMinBounds() const { return minBounds; }
    glm::vec3 getMaxBounds() const { return maxBounds; }
    
    void setRecordingEnabled(bool enabled) { isRecording = enabled; }
    bool isRecordingEnabled() const { return isRecording; }
    void clearHistory();
    void setMaxHistorySize(size_t size) { maxHistorySize = size; }
    
    std::string exportToCSV() const;
    
private:
    std::deque<MassPointData> historyData;
    size_t maxHistorySize;
    
    std::vector<SpringBreakEvent> breakEvents;
    int totalBrokenSprings;
    
    float totalEnergy;
    float averageTension;
    float maxTension;
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
    
    bool isRecording;
    int recordedMassIndex;
    
    std::chrono::steady_clock::time_point startTime;
};

struct AnalysisDisplayData
{
    int selectedMassIndex;
    glm::vec3 position;
    glm::vec3 velocity;
    float speed;
    glm::vec3 acceleration;
    float kineticEnergy;
    int connectedSprings;
    float averageTension;
    
    float totalEnergy;
    float averageSystemTension;
    int totalSprings;
    int brokenSprings;
    float maxTension;
    
    std::vector<float> timeHistory;
    std::vector<float> velocityHistory;
    std::vector<float> energyHistory;
    std::vector<float> tensionHistory;
    
    AnalysisDisplayData() 
        : selectedMassIndex(-1), position(0.0f), velocity(0.0f), 
          speed(0.0f), acceleration(0.0f), kineticEnergy(0.0f),
          connectedSprings(0), averageTension(0.0f),
          totalEnergy(0.0f), averageSystemTension(0.0f),
          totalSprings(0), brokenSprings(0), maxTension(0.0f) {}
};