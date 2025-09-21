//
// Created by qm210 on 21.09.2025.
//

#ifndef DLTROPHY_QMAND_TIMINGSTUFF_H
#define DLTROPHY_QMAND_TIMINGSTUFF_H

#include <chrono>
#include <functional>
#include <optional>

// The chrono namespaces annoy me so much that we stash that stuff in here
namespace chrono = std::chrono;
using steady = chrono::steady_clock;

struct Delayed {
    steady::time_point executeAt;
    std::function<void()> action;
};

struct Measurement {
    steady::time_point start;
    steady::time_point end;
    steady::duration delta{};
    bool done = false;

    Measurement() {
        start = steady::now();
    }

    Measurement (const Measurement&) = default;

    void finish() {
        end = steady::now();
        delta = chrono::duration_cast<chrono::nanoseconds>(end - start);
        done = true;
    }

    [[nodiscard]]
    int deltaNs() const {
        if (!done) {
            throw std::runtime_error("Measurement not finished, cannot ask deltaMs()");
        }
        return delta.count();
    }

    [[nodiscard]]
    float deltaMs() const {
        return 1e-6 * deltaNs();
    }
};

class TimingStuff {
private:
    steady::time_point startedAt;
    steady::time_point currentAt;
    std::optional<Delayed> delayed;
    std::unordered_map<std::string, Measurement> measurements;
    chrono::milliseconds frametime;
    chrono::milliseconds minFrametime;
    chrono::milliseconds maxFrametime;
    chrono::milliseconds avgFrametime;
    int avgSamplesTaken = 0;

public:
    bool printDebug = false;

    TimingStuff() {
        reset();
    }

    void process() {
        auto lastAt = currentAt;
        currentAt = steady::now();
        frametime = chrono::duration_cast<chrono::milliseconds>(currentAt - lastAt);
        minFrametime = std::min(frametime, minFrametime);
        maxFrametime = std::max(frametime, maxFrametime);
        avgFrametime = (avgSamplesTaken * avgFrametime + frametime) / (avgSamplesTaken + 1);
        avgSamplesTaken++;

        if (delayed.has_value()) {
            if (currentAt >= delayed->executeAt) {
                delayed->action();
                delayed = std::nullopt;
            }
        }

        if (printDebug) {
            printDebug = false;
            std::cout << "[DEBUG TIMER] avg frame ms: " << avgFrametime << ", current " << frametime
                      << " in [" << minFrametime << ", " << maxFrametime << "]"
                      << std::endl;
        }
    }

    void executeIn(chrono::milliseconds delay, std::function<void()> action) {
        delayed = {
                .executeAt = steady::now() +
                            (delay.count() > 0 ? delay : chrono::milliseconds::zero()),
                .action = action
        };
    }

    Measurement startMeasurement(const std::string& label) {
        measurements[label] = Measurement{};
        return measurements[label];
    }

    float finishMeasurement(const std::string& label) {
        auto it = measurements.find(label);
        if (it == measurements.end()) {
            // could throw a tantrum, but just ignore for now
            return -1;
        }
        it->second.finish();
        // give measurement result in milliseconds
        return 1e-6 * it->second.deltaNs();
    }

    inline void reset() {
        startedAt = steady::now();
        delayed = std::nullopt;
        frametime = chrono::milliseconds::zero();
        minFrametime = chrono::milliseconds::max();
        maxFrametime = chrono::milliseconds::min();
        avgFrametime = chrono::milliseconds::zero();
        avgSamplesTaken = 0;
    }

    [[nodiscard]]
    static const char* formatted_now() {
        auto system_now = chrono::system_clock::now();
        std::time_t time = chrono::system_clock::to_time_t(system_now);
        // Note: localtime is not thread-safe, but right now I don't want to
        // hassle with Windows localtime_s() vs Linux localtime_r()
        std::tm tm = *std::localtime(&time);
        auto microSec = chrono::duration_cast<chrono::microseconds>(
                system_now.time_since_epoch() % 1000000
        ).count();
        static char buffer[18];
        std::strftime(buffer, 9, "%H:%M:%S", &tm);
        std::snprintf(buffer + 8, 8, ".%06d", static_cast<int>(microSec));
        return buffer;
    }
};

#endif //DLTROPHY_QMAND_TIMINGSTUFF_H
