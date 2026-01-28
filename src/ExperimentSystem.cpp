#include "ExperimentSystem.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

ExperimentLogger::ExperimentLogger()
{
    std::filesystem::create_directories(dataFolder);
}

ExperimentLogger::~ExperimentLogger()
{
    if (isOpen)
    {
        logFile.close();
        isOpen = false;
        std::cout << "Experiment completed: " << experimentName << "\n";
        std::cout << "Data saved to: " << dataFolder << "/" << experimentName << ".csv\n";
    }
}

void ExperimentLogger::startExperiment(const std::string &name)
{
    experimentName = name;
    std::string filename = dataFolder + "/" + experimentName + ".csv";
    logFile.open(filename);

    if (!logFile.is_open())
    {
        std::cerr << "Failed to open log file: " << filename << "\n";
        return;
    }

    logFile << "time,run,totalSprings,brokenSprings,totalEnergy,maxTension,avgTension,totalMasses,avgVelocity,"
               "maxVelocity,height,width,resX,resY,fps\n";
    isOpen = true;
    std::cout << "Started experiment: " << experimentName << "\n";
}

void ExperimentLogger::logFrame(const FrameData &data, int runNumber)
{
    if (!isOpen)
        return;

    logFile << std::fixed << std::setprecision(4);
    logFile << data.time << "," << runNumber << "," << data.totalSprings << "," << data.brokenSprings << ","
            << data.totalEnergy << "," << data.maxTension << "," << data.avgTension << "," << data.totalMasses << ","
            << data.avgVelocity << "," << data.maxVelocity << "," << data.height << "," << data.width << ","
            << data.resX << "," << data.resY << "," << data.fps << "\n";
}

void ExperimentLogger::logEvent(const std::string &message)
{
    if (!isOpen)
        return;
    std::cout << "[" << experimentName << "] " << message << "\n";
}

void ExperimentLogger::endExperiment()
{
    if (isOpen)
    {
        logFile.close();
        isOpen = false;
        std::cout << "Experiment completed: " << experimentName << "\n";
        std::cout << "Data saved to: " << dataFolder << "/" << experimentName << ".csv\n";
    }
}

ExperimentSystem::ExperimentSystem(Cloth *clothPtr) : cloth(clothPtr)
{
}

FrameData ExperimentSystem::collectFrameData(float time, double fps)
{
    FrameData data;
    data.time = time;
    data.fps = fps;

    auto analysisData = cloth->getAnalysisDisplayData();

    data.totalSprings = analysisData.totalSprings;
    data.brokenSprings = analysisData.brokenSprings;
    data.totalEnergy = analysisData.totalEnergy;
    data.maxTension = analysisData.maxTension;
    data.avgTension = analysisData.averageSystemTension;
    data.totalMasses = cloth->getMasses().size();

    float sumVelocity = 0.0f;
    float maxVel = 0.0f;
    const auto &masses = cloth->getMasses();

    for (const auto &mass : masses)
    {
        glm::vec3 vel = mass.position - mass.prevPosition;
        float speed = glm::length(vel);
        sumVelocity += speed;
        maxVel = std::max(maxVel, speed);
    }

    data.avgVelocity = masses.empty() ? 0.0f : sumVelocity / masses.size();
    data.maxVelocity = maxVel;

    data.width = cloth->getClothWidth();
    data.height = cloth->getClothHeight();
    data.resX = 25;
    data.resY = 25;

    return data;
}

void ExperimentSystem::runSimulation(int runNumber, float duration, int logInterval)
{
    std::cout << "  Run #" << runNumber << " starting...\n";

    const float dt = 0.016f;
    float currentTime = 0.0f;
    int frameCount = 0;

    while (currentTime < duration)
    {
        cloth->update(dt);
        currentTime += dt;
        frameCount++;

        if (frameCount % logInterval == 0)
        {
            double fps = 1.0 / dt;
            FrameData data = collectFrameData(currentTime, fps);
            logger.logFrame(data, runNumber);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    std::cout << "  Run #" << runNumber << " completed (" << frameCount << " frames)\n";
}

void ExperimentSystem::exp1_thresholdImpact()
{
    logger.startExperiment("exp1_threshold_impact");
    logger.logEvent("Testing different tension breaking thresholds");

    std::vector<float> thresholds = {2.5f, 3.0f, 3.5f, 4.0f, 5.0f};

    for (int i = 0; i < thresholds.size(); i++)
    {
        float threshold = thresholds[i];
        logger.logEvent("Testing threshold: " + std::to_string(threshold));

        cloth->reset();
        cloth->setEnableTensionBreaking(true);
        cloth->setTensionBreakThreshold(threshold);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(15.0f);
        }

        runSimulation(i, 10.0f, 10);
    }

    logger.endExperiment();
}

void ExperimentSystem::exp2_windStrength()
{
    logger.startExperiment("exp2_wind_strength");
    logger.logEvent("Testing different wind strengths");

    std::vector<float> windStrengths = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};

    for (int i = 0; i < windStrengths.size(); i++)
    {
        float strength = windStrengths[i];
        logger.logEvent("Testing wind strength: " + std::to_string(strength));

        cloth->reset();
        cloth->setEnableTensionBreaking(true);
        cloth->setTensionBreakThreshold(3.5f);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(strength);
            wind->setDirection(glm::vec3(1.0f, 0.0f, 0.0f));
        }

        runSimulation(i, 10.0f, 10);
    }

    logger.endExperiment();
}

void ExperimentSystem::exp3_windDirection()
{
    logger.startExperiment("exp3_wind_direction");
    logger.logEvent("Testing different wind directions");

    std::vector<glm::vec3> directions = {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f)};

    for (int i = 0; i < directions.size(); i++)
    {
        logger.logEvent("Testing wind direction #" + std::to_string(i));

        cloth->reset();
        cloth->setEnableTensionBreaking(true);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(12.0f);
            wind->setDirection(directions[i]);
        }

        runSimulation(i, 10.0f, 10);
    }

    logger.endExperiment();
}

void ExperimentSystem::exp4_gravityImpact()
{
    logger.startExperiment("exp4_gravity_impact");
    logger.logEvent("Testing different gravity values");

    std::vector<float> gravityValues = {0.0f, -5.0f, -9.81f, -15.0f, -20.0f};

    for (int i = 0; i < gravityValues.size(); i++)
    {
        float gravity = gravityValues[i];
        logger.logEvent("Testing gravity: " + std::to_string(gravity));

        cloth->reset();
        cloth->setEnableTensionBreaking(true);

        auto &fm = cloth->getForceManager();
        if (GravityForce *grav = fm.getForce<GravityForce>())
        {
            grav->setEnabled(true);
            grav->setGravity(gravity);
        }

        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(10.0f);
        }

        runSimulation(i, 10.0f, 10);
    }

    logger.endExperiment();
}

void ExperimentSystem::exp5_cascadeBreaking()
{
    logger.startExperiment("exp5_cascade_breaking");
    logger.logEvent("Testing cascade spring breaking behavior");

    for (int run = 0; run < 3; run++)
    {
        logger.logEvent("Cascade test run #" + std::to_string(run));

        cloth->reset();
        cloth->setEnableTensionBreaking(true);
        cloth->setTensionBreakThreshold(3.0f);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(18.0f);
        }

        runSimulation(run, 15.0f, 5);
    }

    logger.endExperiment();
}

void ExperimentSystem::exp6_solverStability()
{
    logger.startExperiment("exp6_solver_stability");
    logger.logEvent("Testing solver iteration impact");

    std::vector<int> iterations = {3, 5, 10, 15, 20};

    for (int i = 0; i < iterations.size(); i++)
    {
        int iter = iterations[i];
        logger.logEvent("Testing solver iterations: " + std::to_string(iter));

        cloth->reset();
        cloth->setSolverParameters(iter, 0.15f, 1.2f);
        cloth->setEnableTensionBreaking(true);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(12.0f);
        }

        runSimulation(i, 10.0f, 10);
    }

    logger.endExperiment();
}

void ExperimentSystem::exp7_meshSizePerf()
{
    logger.startExperiment("exp7_mesh_size_performance");
    logger.logEvent("Testing different mesh resolutions");

    struct MeshConfig
    {
        int resX, resY;
        float width, height;
    };

    std::vector<MeshConfig> configs = {
        {15, 15, 3.0f, 3.0f}, {25, 25, 3.0f, 3.0f}, {35, 35, 5.0f, 5.0f}, {50, 50, 5.0f, 5.0f}};

    for (int i = 0; i < configs.size(); i++)
    {
        auto &config = configs[i];
        logger.logEvent("Testing mesh: " + std::to_string(config.resX) + "x" + std::to_string(config.resY));

        cloth->resize(config.width, config.height, config.resX, config.resY);
        cloth->setEnableTensionBreaking(true);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(10.0f);
        }

        runSimulation(i, 8.0f, 10);
    }

    cloth->resize(3.0f, 3.0f, 25, 25);
    logger.endExperiment();
}

void ExperimentSystem::exp8_springTypes()
{
    logger.startExperiment("exp8_spring_types");
    logger.logEvent("Testing different spring configurations");

    struct SpringConfig
    {
        std::string name;
        float mass, structural, shear, bending;
    };

    std::vector<SpringConfig> configs = {{"soft_silk", 0.3f, 50.0f, 80.0f, 30.0f},
                                         {"normal_fabric", 0.5f, 100.0f, 200.0f, 100.0f},
                                         {"heavy_canvas", 1.5f, 300.0f, 400.0f, 250.0f},
                                         {"rubber_sheet", 0.8f, 150.0f, 150.0f, 80.0f}};

    for (int i = 0; i < configs.size(); i++)
    {
        auto &config = configs[i];
        logger.logEvent("Testing material: " + config.name);

        cloth->setPhysicalProperties(config.mass, config.structural, 1.5f, config.shear, 1.0f, config.bending, 0.8f);
        cloth->reset();
        cloth->setEnableTensionBreaking(true);

        auto &fm = cloth->getForceManager();
        if (WindForce *wind = fm.getForce<WindForce>())
        {
            wind->setEnabled(true);
            wind->setStrength(10.0f);
        }

        runSimulation(i, 10.0f, 10);
    }

    logger.endExperiment();
}

void ExperimentSystem::runAllExp()
{
    std::cout << "\n========================================\n";
    std::cout << "Running ALL experiments\n";
    std::cout << "========================================\n\n";

    exp1_thresholdImpact();
    exp2_windStrength();
    exp3_windDirection();
    exp4_gravityImpact();
    exp5_cascadeBreaking();
    exp6_solverStability();
    exp7_meshSizePerf();
    exp8_springTypes();

    std::cout << "\n========================================\n";
    std::cout << "All experiments completed!\n";
    std::cout << "Results saved in: " << logger.getDataFolder() << "/\n";
    std::cout << "========================================\n\n";
}