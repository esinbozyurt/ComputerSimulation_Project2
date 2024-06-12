#include <iostream>
#include <queue>
#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <iomanip>
#include <string>
#include <unordered_map>

class Event {
public:
    double time;
    std::function<void()> action;

    Event(double time, std::function<void()> action) : time(time), action(action) {}

    bool operator>(const Event& other) const {
        return time > other.time;
    }
};

class Simulation {
private:
    double currentTime;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> eventQueue;

public:
    Simulation() : currentTime(0.0) {}

    void scheduleEvent(double time, std::function<void()> action) {
        eventQueue.push(Event(time, action));
    }

    void run() {
        while (!eventQueue.empty()) {
            Event nextEvent = eventQueue.top();
            eventQueue.pop();
            currentTime = nextEvent.time;
            nextEvent.action();
        }
    }

    double getCurrentTime() const {
        return currentTime;
    }
};

class Machine {
private:
    std::string name;
    Simulation& simulation;
    std::unordered_map<std::string, double> processTimes;
    std::unordered_map<std::string, double> setupTimes;
    std::function<void(const std::string&)> onProcessComplete;
    double breakdownProbability;
    double maintenanceTime;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution;
    bool isBusy;

public:
    Machine(std::string name, Simulation& simulation, std::unordered_map<std::string, double> processTimes, std::unordered_map<std::string, double> setupTimes, std::function<void(const std::string&)> onProcessComplete,
        double breakdownProbability = 0.0, double maintenanceTime = 0.0)
        : name(name), simulation(simulation), processTimes(processTimes), setupTimes(setupTimes), onProcessComplete(onProcessComplete),
        breakdownProbability(breakdownProbability), maintenanceTime(maintenanceTime), distribution(0.0, 1.0), isBusy(false) {}

    void startProcessing(const std::string& productType, double shiftEndTime) {
        if (isBusy || simulation.getCurrentTime() + processTimes[productType] > shiftEndTime) return;

        isBusy = true;
        std::cout << "Machine " << name << " started processing " << productType << " at time " << std::fixed << std::setprecision(2) << simulation.getCurrentTime() << std::endl;
        double totalTime = processTimes[productType] + setupTimes[productType];
        if (distribution(generator) < breakdownProbability) {
            std::cout << "Machine " << name << " broke down! Maintenance required." << std::endl;
            simulation.scheduleEvent(simulation.getCurrentTime() + maintenanceTime, [this, productType, shiftEndTime]() { this->startProcessing(productType, shiftEndTime); });
        }
        else {
            simulation.scheduleEvent(simulation.getCurrentTime() + totalTime, [this, productType]() {
                isBusy = false;
                std::cout << "Machine " << name << " finished processing " << productType << " at time " << std::fixed << std::setprecision(2) << simulation.getCurrentTime() << std::endl;
                onProcessComplete(productType);
                });
        }
    }

    bool isAvailable() const {
        return !isBusy;
    }
};

class ManufacturingSystem {
private:
    Simulation simulation;
    Machine rawMaterialHandler;
    Machine machining;
    Machine assembly;
    Machine qualityControl;
    Machine packaging;
    int productsCompleted;
    int shiftDuration;
    int currentShift;
    int shiftCount;
    double shiftEndTime;
    std::queue<std::string> productQueue;

    void startShift() {
        currentShift++;
        shiftEndTime = simulation.getCurrentTime() + shiftDuration;
        std::cout << "Shift " << currentShift << " started at time " << std::fixed << std::setprecision(2) << simulation.getCurrentTime() << std::endl;
        simulation.scheduleEvent(shiftEndTime, [this]() { this->endShift(); });
        startProduction();
    }

    void endShift() {
        std::cout << "Shift " << currentShift << " ended at time " << std::fixed << std::setprecision(2) << simulation.getCurrentTime() << std::endl;
        if (currentShift < shiftCount) {
            simulation.scheduleEvent(simulation.getCurrentTime(), [this]() { this->startShift(); });
        }
        else {
            printResults();
        }
    }

public:
    ManufacturingSystem(int shiftDuration, int shiftCount)
        : rawMaterialHandler("Raw Material Handler", simulation, { {"ProductA", 2.0}, {"ProductB", 3.0} }, { {"ProductA", 1.0}, {"ProductB", 1.5} }, [this](const std::string& productType) { this->startMachining(productType); }, 0.1, 1.0),
        machining("Machining", simulation, { {"ProductA", 3.0}, {"ProductB", 4.0} }, { {"ProductA", 1.0}, {"ProductB", 2.0} }, [this](const std::string& productType) { this->startAssembly(productType); }, 0.1, 1.5),
        assembly("Assembly", simulation, { {"ProductA", 4.0}, {"ProductB", 5.0} }, { {"ProductA", 1.5}, {"ProductB", 2.5} }, [this](const std::string& productType) { this->startQualityControl(productType); }, 0.1, 2.0),
        qualityControl("Quality Control", simulation, { {"ProductA", 1.0}, {"ProductB", 1.5} }, { {"ProductA", 0.5}, {"ProductB", 1.0} }, [this](const std::string& productType) { this->startPackaging(productType); }, 0.05, 0.5),
        packaging("Packaging", simulation, { {"ProductA", 2.0}, {"ProductB", 2.5} }, { {"ProductA", 0.5}, {"ProductB", 1.0} }, [this](const std::string& productType) { this->finishProduct(productType); }, 0.05, 0.5),
        productsCompleted(0), shiftDuration(shiftDuration), currentShift(0), shiftCount(shiftCount) {
        // Initialize the product queue with alternating products
        for (int i = 0; i < 100; ++i) {
            productQueue.push("ProductA");
            productQueue.push("ProductB");
        }
    }

    void startProduction() {
        if (!productQueue.empty()) {
            std::string productType = productQueue.front();
            productQueue.pop();
            rawMaterialHandler.startProcessing(productType, shiftEndTime);
        }
    }

    void startMachining(const std::string& productType) {
        machining.startProcessing(productType, shiftEndTime);
    }

    void startAssembly(const std::string& productType) {
        assembly.startProcessing(productType, shiftEndTime);
    }

    void startQualityControl(const std::string& productType) {
        qualityControl.startProcessing(productType, shiftEndTime);
    }

    void startPackaging(const std::string& productType) {
        packaging.startProcessing(productType, shiftEndTime);
    }

    void finishProduct(const std::string& productType) {
        productsCompleted++;
        std::cout << productType << " finished at time " << std::fixed << std::setprecision(2) << simulation.getCurrentTime() << std::endl;
        if (simulation.getCurrentTime() < shiftEndTime) {
            startProduction();
        }
    }

    void run() {
        simulation.scheduleEvent(0.0, [this]() { this->startShift(); });
        simulation.run();
    }

    void printResults() const {
        std::cout << "\nSimulation Results:\n";
        std::cout << "-------------------\n";
        std::cout << "Total Products Completed: " << productsCompleted << "\n";
        std::cout << "Total Simulation Time: " << std::fixed << std::setprecision(2) << simulation.getCurrentTime() << "\n";
        std::cout << "-------------------\n";
    }
};

int main() {
    int shiftDuration;
    int shiftCount;

    std::cout << "Enter shift duration (in hours): ";
    std::cin >> shiftDuration;
    std::cout << "Enter number of shifts: ";
    std::cin >> shiftCount;

    ManufacturingSystem system(shiftDuration, shiftCount);
    system.run();

    return 0;
}
