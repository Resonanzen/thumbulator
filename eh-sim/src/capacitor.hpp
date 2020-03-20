#ifndef EH_SIM_CAPACITOR_HPP
#define EH_SIM_CAPACITOR_HPP

#include <cmath>
#include <cassert>
#include <stdexcept>

namespace ehsim {



/**
 * Calculate the energy stored in a capacitor.
 *
 * @param voltage The voltage across the capacitor in volts (V).
 * @param capacitance The capacitance in farads (F).
 *
 * @return The energy in nJ.
 */

class capacitor {
public:
  /**
     * Create a capacitor.
     *
     * @param capacitance The capacitance in farads (F).
     * @param maximum_voltage The maximum voltage the capacitor can sustain.
     */
  capacitor(double const capacitance, double const maximum_voltage, double const maximum_current);

  /**
     * @return The capacitance in farads (F).
     */
  double capacitance() const
  {
    return m_capacitance;
  }

  /**
     * @return The maximum voltage in volts (V).
     */
  double max_voltage() const
  {
    return m_max_voltage;
  }

  /**
     * @return The maximum current in Ampheres (A).
     */
  double max_current() const
  {
    return m_max_current;
  }
  /**
     * @return The maximum amount of energy (J) this capacitor can store.
     */

  double maximum_energy_stored() const
  {
    return m_max_energy;
  }

  /**
     * @return The voltage in volts (V).
     */
  double voltage() const
  {
    return m_voltage;
  }

  void set_voltage(double voltage)
  {
    m_voltage = voltage;
  }

  double charge() const
  {
    return m_charge;
  }

  void set_charge(double charge)
  {
    m_charge = charge;
  }

  /**
   * @return The amount of stored energy in J.
   */
  double energy_stored() const
  {
    return m_energy;
  }

  /**
     * Updates the current voltage across capacitor based on current energy stored
     */
  void update_voltage()
  {
    m_voltage = sqrt(2 * m_energy / m_capacitance);
  }
  /**
   * Updates voltage across capacitor based on current charge it holds
   */
  void update_voltage_v2()
  {
      m_voltage = m_charge / m_capacitance;
  }

  /**
   * Charge the capacitor with the given current (Amperes) for an amount of time (seconds).
   *
   * This will impact the voltage across the capacitor.
   */
  void charge(double current, double time);

  /**
   * Remove charge (Coulombs) from the capacitor.
   *
   * This will impact the voltage across the capacitor.
   */
  void drain(double current, double time);

  /**
  * Consume energy from the capacitor.
  *
  * @param energy_to_consume The amount of energy to consume in J.
  */
  void consume_energy(double const energy_to_consume);

/**
   * Add energy to the capacitor.
   *
   * @param energy_harvested The amount of energy to harvest in J.
   *
   * @return The amount of energy that could be stored.
   */
  double harvest_energy(double const energy_harvested);

private:
  // capacitance
  double const m_capacitance;
  // maximum voltage
  double m_max_voltage;
  // maximum current
  double m_max_current;
  // voltage across the capacitor
  double m_voltage;
  // max energy
  double m_max_energy;
  // stored energy in J
  double m_energy;
  // amount of charge stored on capacitor in C
  double m_charge;

  /**
 * Calculate the energy (Joules) stored in the capacitor.
 */
  inline double calculate_energy(double const voltage, double const capacitance)
  {
    return 0.5 * capacitance * voltage * voltage;
  }

  /**
 * Get the charge (Coulombs) stored in the capacitor.
 */
  inline double calculate_charge(capacitor const &c)
  {
    return c.capacitance() * c.voltage();
  }

  /**
 * Given a load (Amperes and Volts), calculate the time (seconds) to drain the capacitor.
 */
  inline double calculate_time(capacitor const &c, double voltage, double current)
  {
    if(voltage > c.voltage()) {
      return 0.0;
    }
    return calculate_charge(c) / current;
  }
};
} // namespace ehsim

#endif //EH_SIM_CAPACITOR_HPP
