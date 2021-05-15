#pragma once

#include <memory>
#include <string>
#include <vector>

struct PerfAggregator
{
    std::string name_;
    unsigned num_calls_ = 0;
    unsigned total_duration_us_ = 0;
    static std::vector<std::unique_ptr<PerfAggregator>> instances_;

    PerfAggregator(std::string&& name) : name_(name)
    {}

public:
    static PerfAggregator& create(std::string&& name)
    {
        instances_.push_back(std::make_unique<PerfAggregator>(std::move(name)));
        return *instances_.back().get();
    }

    [[nodiscard]] static const std::vector<std::unique_ptr<PerfAggregator>>& get_instances()
    {
        return instances_;
    }

    void add_call(unsigned duration)
    {
        num_calls_++;
        total_duration_us_ += duration;
    }

    [[nodiscard]] const std::string& get_name() const
    {
        return name_;
    }

    [[nodiscard]] unsigned get_calls() const
    {
        return num_calls_;
    }

    [[nodiscard]] unsigned get_total_duration_us() const
    {
        return total_duration_us_;
    }

    [[nodiscard]] unsigned get_avg_duration_us() const
    {
        return total_duration_us_ / num_calls_;
    }
};

class ScopedPerfMonitor
{
    PerfAggregator& agg_;
    unsigned start_ = rf::timer_get(1000000);

public:
    ScopedPerfMonitor(PerfAggregator& agg) : agg_(agg) {}

    ~ScopedPerfMonitor()
    {
        agg_.add_call(rf::timer_get(1000000) - start_);
    }
};
