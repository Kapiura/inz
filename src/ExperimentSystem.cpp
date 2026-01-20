#include "ExperimentSystem.hpp"
#include <filesystem>

ExperimentLogger::ExperimentLogger()
{
    std::filesystem::create_directories(dataFolder);
}

ExperimentLogger::~ExperimentLogger()
{
    if(isOpen)
    {
        logFile.close();
        isOpen = !isOpen;
        std::cout << "Experiment completed: " << experimentName << "\n";
        std::cout << "Data saved to: " << dataFolder << "/" << experimentName << "\n";
    }
}

void ExperimentLogger::startExperiment(const std::string& name)
{
    experimentName = name;
    std::string filename = dataFolder + "/" + experimentName + ".csv";
    logFile.open(filename);

    if(!logFile.is_open())
    {
        std::cerr << "Failed to open log files: " << filename << "\n";
        return;
    }

    logFile << "time,run,totalSprings,brokenSprings,totalEnergy,maxTension,avgTension,totalMasses,avgVelocity,maxVelocity,height,width,resX,resY,fps\n";
    isOpen = true;
    std::cout << "Started experiment: " << experimentName << "\n";
}

void ExperimentLogger::logFrame(const FrameData& data, int runNumber)
{
    if(!isOpen) return;

    logFile << std::fixed << std::setprecision(4);
    logFile << data.time
            << runNumber << ","
            << data.totalSprings << ","
            << data.brokenSprings << ","
            << data.totalEnergy << ","
            << data.maxTension << ","
            << data.avgTension << ","
            << data.totalMasses << ","
            << data.avgVelocity << ","
            << data.maxVelocity << ","
            << data.height << ","
            << data.width << ","
            << data.resX << ","
            << data.resY << ","
            << data.fps << "n";
}

void ExperimentLogger::logEvent(const std::string& message)
{
    if(!isOpen) return;
    std::cout << "[" << experimentName << "] " << message << "\n";
}

void ExperimentLogger::endExperiment()
{
    if(isOpen)
    {
        logFile.close();
        isOpen = !isOpen;
        std::cout << "Experiment completed: " << experimentName << "\n";
        std::cout << "Data saved to: " << dataFolder << "/" << experimentName << "\n";
    }
}

ExperimentSystem::ExperimentSystem(Cloth* clothPtr): cloth(clothPtr) {}

void ExperimentSystem::exp1_thresholdImpact()
{
    std::cout << "exp1\n";
}
void ExperimentSystem::exp2_windStrength()
{
    std::cout << "exp2\n";
}
void ExperimentSystem::exp3_windDirection()
{
    std::cout << "exp3\n";
}
void ExperimentSystem::exp4_gravityImpact()
{
    std::cout << "exp4\n";
}
void ExperimentSystem::exp5_cascadeBreaking()
{
    std::cout << "exp5\n";
}
void ExperimentSystem::exp6_solverStability()
{
    std::cout << "exp6\n";
}
void ExperimentSystem::exp7_meshSizePerf()
{
    std::cout << "exp7\n";
}
void ExperimentSystem::exp8_springTypes()
{
    std::cout << "exp8\n";
}

void ExperimentSystem::runAllExp()
{
    std::cout << "ruynall\n";
}

FrameData ExperimentSystem::collectFrameData(float time)
{
    FrameData data;
    data.time = time;

    auto analysisData = cloth->getAnalysisDisplayData();
    data.
}

// class ExperimentSystem
// {
//     FrameData collectFrameData(float time);
//     void runSimulation(int runNumber, float duration, int logInterval);
// public:
// };