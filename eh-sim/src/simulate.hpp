#ifndef EH_SIM_SIMULATE_HPP
#define EH_SIM_SIMULATE_HPP

#include <chrono>
#include <cstdint>
#include <string>

namespace ehsim {

class eh_scheme;
struct stats_bundle;
class voltage_trace;

/**
 * Simulate an energy harvesting device.
 *
 * @param binary_file The path to the application binary file.
 * @param power The power supply over time.
 * @param scheme The energy harvesting scheme to use.
 *
 * @return The statistics tracked during the simulation.
 */
stats_bundle simulate(char const *binary_file,
    ehsim::voltage_trace const &power,
    eh_scheme *scheme, bool full_sim, uint64_t active_periods_to_simulate, std::string scheme_select);


    std::chrono::nanoseconds get_time(uint64_t const cycle_count, uint32_t const frequency);
    std::chrono::milliseconds to_milliseconds(std::chrono::nanoseconds const &time);



    void update_active_period_stats(ehsim::stats_bundle *stats);
    void update_backup_stats(ehsim::stats_bundle *stats);

}

#endif //EH_SIM_SIMULATE_HPP
