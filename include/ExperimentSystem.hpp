#pragma once

#include <fstream>
#include <ostream>
#include <string>
#include <memory>
#include <vector>

#include "Cloth.hpp"

struct FrameData
{
    float time{0};
    int totalSprings{0};
    int brokenSprings{0};
    float totalEnergy{0};
    float maxTension{0};
    float avgTension{0};
    int totalMasses{0};
    float avgVelocity{0};
    float maxVelocity{0};
    int height{0};
    int width{0};
    int resX{0};
    int resY{0};
    double fps{0};
};

class ExperimentLogger
{
private:
    std::ofstream logFile;
    std::string experimentName;
    std::string dataFolder{"experiment_data"};
    bool isOpen{false};

public:
    ExperimentLogger();
    ~ExperimentLogger();

    void logFrame(const FrameData& data, int runNumber);
    void logEvent(const std::string& message);
    void endExperiment();
    void startExperiment(const std::string& name);

    std::string getDataFolder() const { return dataFolder; };
};

class ExperimentSystem
{
private:
    Cloth* cloth;
    ExperimentLogger logger;

    FrameData collectFrameData(float time, double fps);
    void runSimulation(int runNumber, float duration, int logInterval);
    
public:
    ExperimentSystem(Cloth* clothPtr);

    void exp1_thresholdImpact();
    void exp2_windStrength();
    void exp3_windDirection();
    void exp4_gravityImpact();
    void exp5_cascadeBreaking();
    void exp6_solverStability();
    void exp7_meshSizePerf();
    void exp8_springTypes();

    void runAllExp();
};