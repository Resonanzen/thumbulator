#include <argagg/argagg.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <scheme/magical_scheme.hpp>

#include "scheme/backup_every_cycle.hpp"
#include "scheme/clank.hpp"
#include "scheme/parametric.hpp"
#include "scheme/task_based.hpp"

#include "simulate.hpp"
#include "voltage_trace.hpp"
#include "input_source.hpp"
#include <fstream>


uint32_t task_ram[RAM_SIZE_ELEMENTS];
uint32_t task_flash[FLASH_SIZE_ELEMENTS];


/**
 * Compare text file output of magic/task memory. Need to do this bc declaring 4 arrays to compare flash and ram of both task and magic schemes is too much.
 * @param task_memory_path
 * @return
 */
 //check if two arrays are the sameh
bool memory_is_consistent(uint32_t task_memory[], uint32_t flash_memory[], uint64_t memory_length){
  for (int i = 0; i < memory_length; i++){
      if (task_memory[i] != flash_memory[i]){
          return false;
      }
  }
  return true;
}

void print_usage(std::ostream &stream, argagg::parser const &arguments)
{
  argagg::fmt_ostream help(stream);

  help << "Simulate an energy harvesting environment.\n\n";
  help << "simulate [options] ARG [ARG...]\n\n";
  help << arguments;
}

void ensure_file_exists(std::string const &path_to_file)
{
  std::ifstream binary_file(path_to_file);
  if(!binary_file.good()) {
    throw std::runtime_error("File does not exist: " + path_to_file);
  }
}

void validate(argagg::parser_results const &options)
{
  if(options["binary"].count() == 0) {
    throw std::runtime_error("Missing path to application binary.");
  }

  auto const path_to_binary = options["binary"].as<std::string>();
  ensure_file_exists(path_to_binary);

  if(options["voltages"].count() == 0) {
    throw std::runtime_error("Missing path to voltage trace.");
  }

  auto const path_to_voltage_trace = options["voltages"].as<std::string>();
  ensure_file_exists(path_to_voltage_trace);

  if(options["rate"].count() == 0) {
   // throw std::runtime_error("No sampling rate provided for the voltage trace.");
  }
}
using namespace std::chrono_literals;

int main(int argc, char *argv[])
{
  argagg::parser arguments{{{"help", {"-h", "--help"}, "display help information", 0},
      {"voltages", {"--voltage-trace"}, "path to voltage trace", 1},
      {"rate", {"--voltage-rate"}, "sampling rate of voltage trace (milliseconds)", 1},
      {"harvest", {"--always-harvest"}, "harvest during active periods", 1},
      {"scheme", {"--scheme"}, "the checkpointing scheme to use", 1},
      {"tau_B", {"--tau-b"}, "the backup period for the parametric scheme", 1},
      {"binary", {"-b", "--binary"}, "path to application binary", 1},
      {"output", {"-o", "--output"}, "output file", 1,},
      {"output_folder", {"--output-folder"}, "output folder", 1,},
      {"system_frequency", {"-f", "--frequency"}, "System Frequency", 1,},
      {"active_periods_to_simulate",{"--active-periods"}, "Active Periods to simulate", 1},
      {"check_memory",{"--check-mem"},"check memory consistency", 0}, //check memory consistency within the simulator option
      {"oracle",{"--oracle"},"always back up at ideal time", 0}, //run in oracle state: perfect backup timing
      {"pred",{"--pred"},"predict when power will be lost", 0} //run in simple predict state
  }};



  try {

    auto const options = arguments.parse(argc, argv);
    if (options["help"]) {
      print_usage(std::cout, arguments);
      return EXIT_SUCCESS;
    }

    validate(options);
    //check if check_mem flag has been used
    bool check_mem = false;
    if (options["check_memory"]){
        check_mem = true;
    }

    bool oriko = false;
    if (options["oracle"]){
      oriko = true;
    }

    bool yachi = false;
    if (options["pred"]){
      yachi = true;
    }

    auto const path_to_binary = options["binary"];
    auto const path_to_voltage_trace = options["voltages"];

    std::string output_folder;
    if(options["output_folder"].count() != 0){
      output_folder = options["output_folder"].as<std::string>();
    }

    std::chrono::milliseconds sampling_period(options["rate"]);

    if (options["rate"].count() == 0){
      sampling_period = 1ms;
    }

    bool full_sim = true;
    uint64_t active_periods_to_simulate = 0;
    if (options["active_periods_to_simulate"].count() != 0){
        active_periods_to_simulate = options["active_periods_to_simulate"].as<uint64_t>();
        full_sim = false;
    }

    //double const trace_period = 0.001;
    //double const trace_resistance = 30000;

    //auto input_source = ehsim::input_source(path_to_voltage_trace, trace_period, trace_resistance);


    std::unique_ptr<ehsim::eh_scheme> scheme = nullptr;
    std::string  scheme_select = options["scheme"];
    if(scheme_select == "bec") {
      scheme = std::make_unique<ehsim::backup_every_cycle>();
    } else if(scheme_select == "odab") {
      throw std::runtime_error("ODAB is no longer supported.");
    } else if(scheme_select == "magic") {
      scheme = std::make_unique<ehsim::magical_scheme>();
    } else if(scheme_select == "clank") {
      scheme = std::make_unique<ehsim::clank>();
    } else if(scheme_select == "parametric") {
      auto const tau_b = options["tau_B"].as<int>(1000);
      scheme = std::make_unique<ehsim::parametric>(tau_b);
    } else if(scheme_select == "task") {



      if (options["system_frequency"].count() == 0){
        scheme = std::make_unique<ehsim::task_based>();
      }else {
        scheme = std::make_unique<ehsim::task_based>(options["system_frequency"].as<uint64_t>());
      }

    } else {
      throw std::runtime_error("Unknown scheme selected.");
    }

    ehsim::voltage_trace power(path_to_voltage_trace, sampling_period);


    std::cout <<"Running with scheme " << scheme_select << ":\n";
    auto const stats = ehsim::simulate(path_to_binary, output_folder, power, scheme.get(), full_sim, oriko, yachi, active_periods_to_simulate);
    //copy memory into separate vector, so it can be compared against magic laterd
    std::copy(std::begin(thumbulator::RAM), std::end(thumbulator::RAM), std::begin(task_ram));
    std::copy(std::begin(thumbulator::FLASH_MEMORY), std::end(thumbulator::FLASH_MEMORY), std::begin(task_flash));

    std::string output_file_name(scheme_select + ".csv");
    if(options["output"].count() > 0) {
      output_file_name = options["output"].as<std::string>();
    }

    std::ofstream out(output_file_name);
    out.setf(std::ios::fixed);
    out << "id, E, epsilon, epsilon_C, tau_B, alpha_B, energy_consumed, n_B, tau_P, tau_D, e_P, e_B, "
           "e_R, sim_p, eh_p\n";

    int id = 0;
    for(auto const &model : stats.models) {
      out << id++ << ", ";

      auto const eh_parameters = ehsim::eh_model_parameters(model);
      out << std::setprecision(3) << eh_parameters.E << ", ";
      out << std::setprecision(3) << eh_parameters.epsilon << ", ";
      out << std::setprecision(3) << eh_parameters.epsilon_C << ", ";
      out << std::setprecision(2) << eh_parameters.tau_B << ", ";
      out << std::setprecision(4) << eh_parameters.alpha_B << ", ";

      auto const tau_D = model.time_for_instructions - model.time_forward_progress;
      out << std::setprecision(3) << model.energy_consumed << ", ";
      out << std::setprecision(0) << model.num_backups << ", ";
      out << std::setprecision(0) << model.time_forward_progress << ", ";
      out << std::setprecision(0) << tau_D << ", ";
      out << std::setprecision(3) << model.energy_forward_progress << ", ";
      out << std::setprecision(3) << model.energy_for_backups << ", ";
      out << std::setprecision(3) << model.energy_for_restore << ", ";

      out << std::setprecision(3) << model.progress << ", ";
      out << std::setprecision(3) << model.eh_progress << "\n";
    }


    //re-run the program with the magic scheme to compare memory

    if (check_mem) {
        std::cout << "\nRunning simulation with magic scheme: \n";
        std::unique_ptr<ehsim::eh_scheme> magic_scheme = nullptr;
        magic_scheme = std::make_unique<ehsim::magical_scheme>();
        auto const stats =   ehsim::simulate(path_to_binary, output_folder, power, magic_scheme.get(), full_sim, oriko, yachi, active_periods_to_simulate);



        //check the memory consistency
        std::cout << "Checking memory consistency..\n";
        if (memory_is_consistent(task_ram, thumbulator::RAM, RAM_SIZE_ELEMENTS) && memory_is_consistent(task_flash, thumbulator::FLASH_MEMORY, FLASH_SIZE_ELEMENTS)){
            std::cout << "Memory is consistent!\n";
        }else{
            std::cout << "Memory is not consistent! Please run: diff task_memory_dump magic_memory_dump \n";
        }


    }
  } catch(std::exception const &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}
