#ifndef _I_METRICS_SCOPE_H
#define _I_METRICS_SCOPE_H

#include <metrics/IMetricsInstruments.hpp>

namespace metrics_manager 
{

class IMetricsScope 
{
public:
    virtual std::shared_ptr<iCounter<double>> getCounterDouble(const std::string& name) = 0;
    virtual std::shared_ptr<iCounter<uint64_t>> getCounterInteger(const std::string& name) = 0;
    virtual std::shared_ptr<iCounter<double>> getUpDownCounterDouble(const std::string& name) = 0;
    virtual std::shared_ptr<iCounter<int64_t>> getUpDownCounterInteger(const std::string& name) = 0;
    virtual std::shared_ptr<iHistogram<double>> getHistogramDouble(const std::string& name) = 0;
    virtual std::shared_ptr<iHistogram<uint64_t>> getHistogramInteger(const std::string& name) = 0;
};

} // namespace metrics_manager

#endif // _I_METRICS_SCOPE_H