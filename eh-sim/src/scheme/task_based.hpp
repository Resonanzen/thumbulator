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
    /**
     * constructor that can affect the frequency
     * @param __system_frequency
     */
    task_based(uint64_t __system_frequency) : battery(MEMENTOS_CAPACITANCE, MEMENTOS_MAX_CAPACITOR_VOLTAGE, MEMENTOS_MAX_CURRENT)
    {
        system_frequency = __system_frequency;
    }

    /**
     * return the battery that is associated with the system
     * @return capacitor
     */

    capacitor &get_battery() override
    {
        return battery;
    }
    /**
     * return the current clock frequency
     * @return uint32_t
     */
    uint32_t clock_frequency() const override
    {
        return system_frequency;
    }
    //should delete from  base class, not really needed
    double min_energy_to_power_on(stats_bundle *stats) override
    {
        auto required_energy = NVP_INSTRUCTION_ENERGY + NVP_BEC_BACKUP_ENERGY;

        if(stats->cpu.instruction_count != 0) {
            // we only need to restore if an instruction has been executed
            required_energy += NVP_BEC_RESTORE_ENERGY;
        }
        return required_energy;
    }

    /**
     *  Update energy usage after instruction is executed. The acutual exeuction of the instruction is performed in step_cpu
     * @param cycles
     * @param stats
     */
    void execute_instruction(uint32_t cycles,stats_bundle *stats){

        double energy_to_consume = cycles * CORTEX_M0PLUS_INSTRUCTION_ENERGY_PER_CYCLE;

        battery.consume_energy(energy_to_consume);
       // std::cout <<energy_to_consume << "\n";
        stats->models.back().energy_for_instructions +=energy_to_consume;
    }
    //Not used, potentially can delete
    void execute_instruction(stats_bundle *stats) override
    {
        //TODO: currently every inst consumes the same energy regardless of cycles (ticks)

        auto const elapsed_cycles = stats->cpu.cycle_count - last_tick;
        last_tick = stats->cpu.cycle_count;


        //std::cout << elapsed_cycles << std::endl;
        //not correct..maybe should use CPU cycle
        battery.consume_energy(elapsed_cycles*CORTEX_M0PLUS_INSTRUCTION_ENERGY_PER_CYCLE);

        stats->models.back().energy_for_instructions += CORTEX_M0PLUS_INSTRUCTION_ENERGY_PER_CYCLE*elapsed_cycles;
    }
    /**
     * Return whether or not the system is active
     * @param stats
     * @return bool
     */
    bool is_active(stats_bundle *stats) override
    {
        //yeah... unsure about required energy
        //I am just going to use the STM32 minimum operating voltage (1.5V) to set the required energy (datasheet in data_sheet.hpp)
        //this is still bad bc our ENERGY/cycle calculations assumes that the voltage across the CPU is constant
        auto required_energy =  0.5 * battery.capacitance() * (CORTEX_MOPLUS_MINIMUM_OPERATING_VOLTAGE * CORTEX_MOPLUS_MINIMUM_OPERATING_VOLTAGE);//32*CORTEX_M0PLUS_INSTRUCTION_ENERGY_PER_CYCLE + CORTEX_M0PLUS_ENERGY_FLASH*(100);  //32 is max number of cycles for customTest (multiply instruction takes 32  cycles!!!)
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
    /**
     * Return whether or not system will backup after encountering a WFI instruction
     * @param stats
     * @return
     */
    bool will_backup(stats_bundle *stats) const override
    {
        return stats->backup_requested;
    }

    /**
     * Perform backup. RAM, FLASH, registers are all saved. Update energy conditions after backup
     * @param stats
     * @return uint64_t (num cycles the backup took)
     */
    uint64_t backup(stats_bundle *stats) override
    {

        /*
         * Assume that this system keeps track of which memory locations are used and which aren't, which means you only need to backup
         * the RAM addresses that are important for the program.
         *
         * This is needed bc the energy needed to backup 8mb of RAM is way higher than the current capacitor's capacity
         */

       // std::cout << "BACKUP! Saving state at pc = " << std::hex<< thumbulator::cpu_get_pc() - 0x5 << "\n";
        // do not touch arch/app state, assume it is all non-volatile
        auto &active_stats = stats->models.back();
        active_stats.num_backups++;
        branch_taken = thumbulator::BRANCH_WAS_TAKEN;
        active_stats.time_between_backups += stats->cpu.cycle_count - last_backup_cycle;
        last_backup_cycle = stats->cpu.cycle_count;

        std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(backup_RAM));
        std::copy(std::begin(thumbulator::FLASH_MEMORY), std::end(thumbulator::FLASH_MEMORY), std::begin(backup_FLASH));
        backup_ARCHITECTURE = thumbulator::cpu;

        double energy_for_backup =  CORTEX_M0PLUS_ENERGY_FLASH*(thumbulator::used_RAM_addresses.size());
        //std::cout <<"Bytes to backup: " << thumbulator::used_RAM_addresses.size() << "\n";
        active_stats.energy_for_backups += energy_for_backup ;

        battery.consume_energy(energy_for_backup);
        //return the number of cycles to backup al the used parts of RAM
        return (thumbulator::used_RAM_addresses.size())* TIMING_MEM;
    }
    /**
     * Restore system to the previous backed up state
     * @param stats
     * @return
     */
    uint64_t restore(stats_bundle *stats) override
    {
        last_backup_cycle = stats->cpu.cycle_count;
        thumbulator::BRANCH_WAS_TAKEN = branch_taken;

        // do not touch arch/app state, assume it is all non-volatile
     std::copy(std::begin(backup_RAM), std::end(backup_RAM), std::begin(thumbulator::RAM));
     std::copy(std::begin(backup_FLASH), std::end(backup_FLASH), std::begin(thumbulator::FLASH_MEMORY));

        thumbulator::cpu_reset();
       thumbulator::cpu = backup_ARCHITECTURE;
        std::cout << "RESTORE! Restore at PC = "<< std::hex << thumbulator::cpu_get_pc() -0x5 << "\n";
        std::cout << std::dec;
        double energy_for_restore =  CORTEX_M0PLUS_ENERGY_FLASH*(thumbulator::used_RAM_addresses.size());
        //only track stores between backups
        thumbulator::used_RAM_addresses.clear();
        stats->models.back().energy_for_restore = energy_for_restore;
        battery.consume_energy(energy_for_restore);

        return (thumbulator::used_RAM_addresses.size())*TIMING_MEM;
    }

    double estimate_progress(eh_model_parameters const &eh) const override
    {
        return estimate_eh_progress(eh, dead_cycles::best_case, NVP_BEC_OMEGA_R, NVP_BEC_SIGMA_R, NVP_BEC_A_R,
                                    NVP_BEC_OMEGA_B, NVP_BEC_SIGMA_B, NVP_BEC_A_B);
    }

    /**
     * Ensure that there is an existing state to fall back on from the verying beginning.
     */

    void execute_startup_routine()  override
    {

        std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(backup_RAM));
        std::copy(std::begin(thumbulator::FLASH_MEMORY), std::end(thumbulator::FLASH_MEMORY), std::begin(backup_FLASH));
        backup_ARCHITECTURE = thumbulator::cpu;
    }



private:
    capacitor battery;
    bool active;
    uint64_t last_tick = 0u;
    uint64_t last_backup_cycle = 0u;
    uint32_t backup_RAM[RAM_SIZE_BYTES >> 2];
    uint32_t backup_FLASH[FLASH_SIZE_BYTES>>2];
    thumbulator::cpu_state backup_ARCHITECTURE = thumbulator::cpu;
    uint64_t system_frequency = CORTEX_M0PLUS_FREQUENCY;
    bool branch_taken = thumbulator::BRANCH_WAS_TAKEN;
    uint64_t ram_bytes_used = 0;
};
}

#endif //EH_SIM_TASK_BASED_HPP
