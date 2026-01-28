#pragma once

#include <chrono>
#include <deque>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Mass;
struct Spring;

struct MassPointData
{
    // Time
    float time;
    // Positions data
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    // Force and energy
    float kineticEnergy;
    glm::vec3 totalForce;

    // Spring data
    float averageSpringTension;
    int connectedSprings;

    MassPointData()
        : time(0.0f), position(0.0f), velocity(0.0f), acceleration(0.0f), totalForce(0.0f), kineticEnergy(0.0f),
          averageSpringTension(0.0f), connectedSprings(0)
    {
    }
};

// Record broken springs
struct SpringBreakEvent
{
    float time;
    float tension;
    int springIndex;
    glm::vec3 position;

    SpringBreakEvent(float t, int idx, const glm::vec3 &pos, float tens)
        : time(t), springIndex(idx), position(pos), tension(tens)
    {
    }
};

class ClothAnalysis
{
  public:
    ClothAnalysis();

    void update(float deltaTime, float simulationTime);
    void recordMassPointData(int massIndex, const Mass &mass, const std::vector<Spring> &springs, float simulationTime);
    void recordSpringBreak(int springIndex, const glm::vec3 &position, float tension, float simulationTime);

    float calculateKineticEnergy(const Mass &mass) const;
    float calculateAverageSpringTension(int massIndex, const std::vector<Mass> &masses,
                                        const std::vector<Spring> &springs) const;
    glm::vec3 calculateVelocity(const Mass &mass) const;

    void updateGlobalStats(const std::vector<Mass> &masses, const std::vector<Spring> &springs, float simulationTime);

    // Get history
    const std::deque<MassPointData> &getHistoryData() const
    {
        return historyData;
    }
    const std::vector<SpringBreakEvent> &getBreakEvents() const
    {
        return breakEvents;
    }

    // Get stats
    float getTotalEnergy() const
    {
        return totalEnergy;
    }
    float getAverageTension() const
    {
        return averageTension;
    }
    int getTotalBrokenSprings() const
    {
        return totalBrokenSprings;
    }
    float getMaxTension() const
    {
        return maxTension;
    }
    glm::vec3 getMinBounds() const
    {
        return minBounds;
    }
    glm::vec3 getMaxBounds() const
    {
        return maxBounds;
    }

    // Control
    void setRecordingEnabled(bool enabled)
    {
        isRecording = enabled;
    }
    bool isRecordingEnabled() const
    {
        return isRecording;
    }
    void clearHistory();
    void setMaxHistorySize(size_t size)
    {
        maxHistorySize = size;
    }

    std::string exportToCSV() const;

  private:
    // Record data
    std::deque<MassPointData> historyData;
    std::vector<SpringBreakEvent> breakEvents;
    std::chrono::steady_clock::time_point startTime;

    // Current stats
    float totalEnergy;
    float averageTension;
    float maxTension;
    int totalBrokenSprings;

    // Cloth boundaries
    glm::vec3 minBounds;
    glm::vec3 maxBounds;

    // Record settings
    size_t maxHistorySize;
    bool isRecording;
    int recordedMassIndex;
};

struct AnalysisDisplayData
{
    // Mass data
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    int selectedMassIndex;
    float speed;
    // Energy
    float totalEnergy;
    float kineticEnergy;
    // Spring data
    float maxTension;
    float averageTension;
    float averageSystemTension;
    int totalSprings;
    int brokenSprings;
    int connectedSprings;

    // History for graphs
    std::vector<float> timeHistory;
    std::vector<float> energyHistory;
    std::vector<float> tensionHistory;
    std::vector<float> velocityHistory;

    AnalysisDisplayData()
        : selectedMassIndex(-1), position(0.0f), velocity(0.0f), speed(0.0f), acceleration(0.0f), kineticEnergy(0.0f),
          connectedSprings(0), averageTension(0.0f), totalEnergy(0.0f), averageSystemTension(0.0f), totalSprings(0),
          brokenSprings(0), maxTension(0.0f)
    {
    }
};