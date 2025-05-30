/* chombo-discharge
 * Copyright © 2022 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_KMCDualStateImplem.H
  @brief  Implementation of CD_KMCDualState.H
  @author Robert Marskar
*/

#ifndef CD_KMCDualStateImplem_H
#define CD_KMCDualStateImplem_H

// Our includes
#include <CD_KMCDualState.H>
#include <CD_NamespaceHeader.H>

template <typename T>
KMCDualState<T>::KMCDualState(const size_t a_numReactiveSpecies, const size_t a_numNonReactiveSpecies) noexcept
{
  CH_TIME("KMCDualState::KMCDualState");

  this->define(a_numReactiveSpecies, a_numNonReactiveSpecies);

  CH_assert(m_reactiveState.size() > 0);
}

template <typename T>
inline KMCDualState<T>::~KMCDualState()
{}

template <typename T>
inline void
KMCDualState<T>::define(const size_t a_numReactiveSpecies, const size_t a_numNonReactiveSpecies) noexcept
{
  CH_TIME("KMCDualState::define");

  m_numReactiveSpecies    = a_numReactiveSpecies;
  m_numNonReactiveSpecies = a_numNonReactiveSpecies;

  m_reactiveState.resize(a_numReactiveSpecies);
  m_nonReactiveState.resize(a_numNonReactiveSpecies);
}

template <typename T>
inline bool
KMCDualState<T>::isValidState() const noexcept
{
  CH_TIME("KMCDualState::isValidState");

  bool isValid = true;

  for (const auto& p : m_reactiveState) {
    if (p < 0) {
      isValid = false;

      break;
    }
  }

  for (const auto& p : m_nonReactiveState) {
    if (p < 0) {
      isValid = false;

      break;
    }
  }

  return isValid;
}

template <typename T>
inline std::vector<T>
KMCDualState<T>::linearOut() const noexcept
{
  std::vector<T> out(0);

  out.insert(out.end(), m_reactiveState.begin(), m_reactiveState.end());
  out.insert(out.end(), m_nonReactiveState.begin(), m_nonReactiveState.end());

  return out;
}

template <typename T>
inline void
KMCDualState<T>::linearIn(const std::vector<T>& a_linearizedState) noexcept
{
  for (size_t i = 0; i < m_numReactiveSpecies; i++) {
    m_reactiveState[i] = a_linearizedState[i];
  }

  for (size_t i = 0; i < m_numNonReactiveSpecies; i++) {
    m_nonReactiveState[i] = a_linearizedState[m_numReactiveSpecies + i];
  }
}

template <typename T>
inline typename KMCDualState<T>::State&
KMCDualState<T>::getReactiveState() noexcept
{
  return m_reactiveState;
}

template <typename T>
inline const typename KMCDualState<T>::State&
KMCDualState<T>::getReactiveState() const noexcept
{
  return m_reactiveState;
}

template <typename T>
inline typename KMCDualState<T>::State&
KMCDualState<T>::getNonReactiveState() noexcept
{
  return m_nonReactiveState;
}

template <typename T>
inline const typename KMCDualState<T>::State&
KMCDualState<T>::getNonReactiveState() const noexcept
{
  return m_nonReactiveState;
}

template <typename T>
inline std::ostream&
operator<<(std::ostream& ostr, const KMCDualState<T>& a_state)
{
  ostr << "KMCDualState : \n";

  // Print position.
  const auto& reactiveState    = a_state.getReactiveState();
  const auto& nonReactiveState = a_state.getNonReactiveState();

  ostr << "\t Reactive     = (";
  for (size_t i = 0; i < reactiveState.size(); i++) {
    ostr << reactiveState[i];
    if (i < reactiveState.size() - 1) {
      ostr << ",";
    }
  }
  ostr << ")\n";

  ostr << "\t Non-reactive = (";
  for (size_t i = 0; i < nonReactiveState.size(); i++) {
    ostr << nonReactiveState[i];
    if (i < nonReactiveState.size() - 1) {
      ostr << ",";
    }
  }
  ostr << ")\n";

  return ostr;
}

#include <CD_NamespaceFooter.H>

#endif
