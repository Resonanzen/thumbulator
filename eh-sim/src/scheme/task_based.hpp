#ifndef EH_SIM_TASK_BASED_HPP
#define EH_SIM_TASK_BASED_HPP

#include "scheme/eh_scheme.hpp"
#include "scheme/data_sheet.hpp"
#include "scheme/eh_model.hpp"
#include "capacitor.hpp"

namespace ehsim {

/**
 * Tester scheme for dual purpose predictor
 *
 */
class task_based : public eh_scheme {
public:
    task_based() : battery(MEMENTOS_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
    {
    }

    task_based(uint64_t __system_frequency) : battery(MEMENTOS_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
    {
        system_frequency = __system_frequency;
    }



    capacitor &get_battery() override
    {
        return battery;
    }

    uint32_t clock_frequency() const override
    {
        return system_frequency;
    }

    double min_energy_to_power_on(stats_bundle *stats) override
    {
        auto required_energy = NVP_INSTRUCTION_ENERGY + NVP_BEC_BACKUP_ENERGY;

        if(stats->cpu.instruction_count != 0) {
            // we only need to restore if an instruction has been executed
            required_energy += NVP_BEC_RESTORE_ENERGY;
        }
        return required_energy;
    }

    void execute_instruction(stats_bundle *stats) override
    {
        //TODO: currently every inst consumes the same energy regardless of cycles (ticks)

        auto const elapsed_cycles = stats->cpu.cycle_count - last_tick;
        last_tick = stats->cpu.cycle_count;


        //std::cout << elapsed_cycles << std::endl;
        //not correct..maybe should use CPU cycle
        battery.consume_energy(elapsed_cycles*CORTEX_M0PLUS_INSTRUCTION_ENERGY_PER_CYCLE);

        stats->models.back().energy_for_instructions += NVP_INSTRUCTION_ENERGY*elapsed_cycles;
    }

    bool is_active(stats_bundle *stats) override
    {
        auto required_energy = 32*CORTEX_M0PLUS_INSTRUCTION_ENERGY_PER_CYCLE + NVP_BEC_BACKUP_ENERGY + NVP_BEC_RESTORE_ENERGY;  //32 is max number of cycles for customTest (multiply instruction takes 32  cycles!!!)
        if(battery.energy_stored() >  battery.maximum_energy_stored() - required_energy)
        {
            active = true;
        }
        else if (battery.energy_stored() < required_energy)
        {

            active = false;
        }
        return active;
    }

    bool will_backup(stats_bundle *stats) const override
    {
        return stats->backup_requested;
    }


    uint64_t backup(stats_bundle *stats) override
    {

        std::cout << "Backup: " << stats->system.time.count() *1E-9 << " " << battery.energy_stored() << "\n";


        //std::cout << "BACKUP! Saving state at pc = " << thumbulator::cpu_get_pc() - 0x5 << "\n";
        // do not touch arch/app state, assume it is all non-volatile
        auto &active_stats = stats->models.back();
        active_stats.num_backups++;

        active_stats.time_between_backups += stats->cpu.cycle_count - last_backup_cycle;
        last_backup_cycle = stats->cpu.cycle_count;

        std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(backup_RAM));
        std::copy(std::begin(thumbulator::FLASH_MEMORY), std::end(thumbulator::FLASH_MEMORY), std::begin(backup_FLASH));
        backup_ARCHITECTURE = thumbulator::cpu;
        active_stats.energy_for_backups += NVP_BEC_BACKUP_ENERGY;
        battery.consume_energy(NVP_BEC_BACKUP_ENERGY);

        return NVP_BEC_BACKUP_TIME;
    }

    uint64_t restore(stats_bundle *stats) override
    {
        last_backup_cycle = stats->cpu.cycle_count;


        // do not touch arch/app state, assume it is all non-volatile
        std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(backup_RAM));
        std::copy(std::begin(backup_FLASH), std::end(backup_FLASH), std::begin(thumbulator::FLASH_MEMORY));
        thumbulator::cpu = backup_ARCHITECTURE;
        std::cout << "RESTORE! Restore at PC = "<< thumbulator::cpu_get_pc() -0x5 << "\n";
        stats->models.back().energy_for_restore = NVP_BEC_RESTORE_ENERGY;
        battery.consume_energy(NVP_BEC_RESTORE_ENERGY);

        return NVP_BEC_RESTORE_TIME;
    }

    double estimate_progress(eh_model_parameters const &eh) const override
    {
        return estimate_eh_progress(eh, dead_cycles::best_case, NVP_BEC_OMEGA_R, NVP_BEC_SIGMA_R, NVP_BEC_A_R,
                                    NVP_BEC_OMEGA_B, NVP_BEC_SIGMA_B, NVP_BEC_A_B);
    }

private:
    capacitor battery;
    bool active;
    uint64_t last_tick = 0u;
    uint64_t last_backup_cycle = 0u;
    uint32_t backup_RAM[RAM_SIZE_BYTES >> 2];
    uint32_t backup_FLASH[RAM_SIZE_BYTES >>2];
    thumbulator::cpu_state backup_ARCHITECTURE;
    uint64_t system_frequency = CORTEX_M0PLUS_FREQUENCY;
};
}

#endif //EH_SIM_TASK_BASED_HPP
