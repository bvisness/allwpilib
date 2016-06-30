/*----------------------------------------------------------------------------*/
/* Copyright (c) FIRST 2008-2016. All Rights Reserved.                        */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "Relay.h"

#include "HAL/HAL.h"
#include "LiveWindow/LiveWindow.h"
#include "MotorSafetyHelper.h"
#include "Resource.h"
#include "WPIErrors.h"

#include <sstream>

// Allocate each direction separately.
static std::unique_ptr<Resource> relayChannels;

/**
 * Relay constructor given a channel.
 *
 * This code initializes the relay and reserves all resources that need to be
 * locked. Initially the relay is set to both lines at 0v.
 *
 * @param channel   The channel number (0-3).
 * @param direction The direction that the Relay object will control.
 */
Relay::Relay(uint32_t channel, Relay::Direction direction)
    : m_channel(channel), m_direction(direction) {
  std::stringstream buf;
  if (!SensorBase::CheckRelayChannel(m_channel)) {
    buf << "Relay Channel " << m_channel;
    wpi_setWPIErrorWithContext(ChannelIndexOutOfRange, buf.str());
    return;
  }

  HalPortHandle portHandle = getPort(channel);

  if (m_direction == kBothDirections || m_direction == kForwardOnly) {
    int32_t status = 0;
    m_forwardHandle = initializeRelayPort(portHandle, true, &status);
    if (status != 0) {
      wpi_setErrorWithContext(status, getHALErrorMessage(status));
      m_forwardHandle = HAL_INVALID_HANDLE;
      m_reverseHandle = HAL_INVALID_HANDLE;
      return;
    }
    HALReport(HALUsageReporting::kResourceType_Relay, m_channel);
  }
  if (m_direction == kBothDirections || m_direction == kReverseOnly) {
    int32_t status = 0;
    m_reverseHandle = initializeRelayPort(portHandle, false, &status);
    if (status != 0) {
      wpi_setErrorWithContext(status, getHALErrorMessage(status));
      m_forwardHandle = HAL_INVALID_HANDLE;
      m_reverseHandle = HAL_INVALID_HANDLE;
      return;
    }

    HALReport(HALUsageReporting::kResourceType_Relay, m_channel + 128);
  }

  int32_t status = 0;
  if (m_forwardHandle != HAL_INVALID_HANDLE) {
    setRelay(m_forwardHandle, false, &status);
    if (status != 0) {
      wpi_setErrorWithContext(status, getHALErrorMessage(status));
      m_forwardHandle = HAL_INVALID_HANDLE;
      m_reverseHandle = HAL_INVALID_HANDLE;
      return;
    }
  }
  if (m_reverseHandle != HAL_INVALID_HANDLE) {
    setRelay(m_reverseHandle, false, &status);
    if (status != 0) {
      wpi_setErrorWithContext(status, getHALErrorMessage(status));
      m_forwardHandle = HAL_INVALID_HANDLE;
      m_reverseHandle = HAL_INVALID_HANDLE;
      return;
    }
  }

  m_safetyHelper = std::make_unique<MotorSafetyHelper>(this);
  m_safetyHelper->SetSafetyEnabled(false);

  LiveWindow::GetInstance()->AddActuator("Relay", 1, m_channel, this);
}

/**
 * Free the resource associated with a relay.
 *
 * The relay channels are set to free and the relay output is turned off.
 */
Relay::~Relay() {
  int32_t status = 0;
  setRelay(m_forwardHandle, false, &status);
  setRelay(m_reverseHandle, false, &status);
  // ignore errors, as we want to make sure a free happens.
  if (m_forwardHandle != HAL_INVALID_HANDLE) freeRelayPort(m_forwardHandle);
  if (m_reverseHandle != HAL_INVALID_HANDLE) freeRelayPort(m_reverseHandle);

  if (m_table != nullptr) m_table->RemoveTableListener(this);
}

/**
 * Set the relay state.
 *
 * Valid values depend on which directions of the relay are controlled by the
 * object.
 *
 * When set to kBothDirections, the relay can be any of the four states:
 * 0v-0v, 0v-12v, 12v-0v, 12v-12v
 *
 * When set to kForwardOnly or kReverseOnly, you can specify the constant for
 * the direction or you can simply specify kOff and kOn.  Using only kOff and
 * kOn is recommended.
 *
 * @param value The state to set the relay.
 */
void Relay::Set(Relay::Value value) {
  if (StatusIsFatal()) return;

  int32_t status = 0;

  switch (value) {
    case kOff:
      if (m_direction == kBothDirections || m_direction == kForwardOnly) {
        setRelay(m_forwardHandle, false, &status);
      }
      if (m_direction == kBothDirections || m_direction == kReverseOnly) {
        setRelay(m_reverseHandle, false, &status);
      }
      break;
    case kOn:
      if (m_direction == kBothDirections || m_direction == kForwardOnly) {
        setRelay(m_forwardHandle, true, &status);
      }
      if (m_direction == kBothDirections || m_direction == kReverseOnly) {
        setRelay(m_reverseHandle, true, &status);
      }
      break;
    case kForward:
      if (m_direction == kReverseOnly) {
        wpi_setWPIError(IncompatibleMode);
        break;
      }
      if (m_direction == kBothDirections || m_direction == kForwardOnly) {
        setRelay(m_forwardHandle, true, &status);
      }
      if (m_direction == kBothDirections) {
        setRelay(m_reverseHandle, false, &status);
      }
      break;
    case kReverse:
      if (m_direction == kForwardOnly) {
        wpi_setWPIError(IncompatibleMode);
        break;
      }
      if (m_direction == kBothDirections) {
        setRelay(m_forwardHandle, false, &status);
      }
      if (m_direction == kBothDirections || m_direction == kReverseOnly) {
        setRelay(m_reverseHandle, true, &status);
      }
      break;
  }

  wpi_setErrorWithContext(status, getHALErrorMessage(status));
}

/**
 * Get the Relay State
 *
 * Gets the current state of the relay.
 *
 * When set to kForwardOnly or kReverseOnly, value is returned as kOn/kOff not
 * kForward/kReverse (per the recommendation in Set)
 *
 * @return The current state of the relay as a Relay::Value
 */
Relay::Value Relay::Get() const {
  int32_t status;

  if (getRelay(m_forwardHandle, &status)) {
    if (getRelay(m_reverseHandle, &status)) {
      return kOn;
    } else {
      if (m_direction == kForwardOnly) {
        return kOn;
      } else {
        return kForward;
      }
    }
  } else {
    if (getRelay(m_reverseHandle, &status)) {
      if (m_direction == kReverseOnly) {
        return kOn;
      } else {
        return kReverse;
      }
    } else {
      return kOff;
    }
  }

  wpi_setErrorWithContext(status, getHALErrorMessage(status));
}

uint32_t Relay::GetChannel() const { return m_channel; }

/**
 * Set the expiration time for the Relay object
 * @param timeout The timeout (in seconds) for this relay object
 */
void Relay::SetExpiration(float timeout) {
  m_safetyHelper->SetExpiration(timeout);
}

/**
 * Return the expiration time for the relay object.
 * @return The expiration time value.
 */
float Relay::GetExpiration() const { return m_safetyHelper->GetExpiration(); }

/**
 * Check if the relay object is currently alive or stopped due to a timeout.
 *
 * @return a bool value that is true if the motor has NOT timed out and should
 *         still be running.
 */
bool Relay::IsAlive() const { return m_safetyHelper->IsAlive(); }

/**
 * Stop the motor associated with this PWM object.
 *
 * This is called by the MotorSafetyHelper object when it has a timeout for this
 * relay and needs to stop it from running.
 */
void Relay::StopMotor() { Set(kOff); }

/**
 * Enable/disable motor safety for this device.
 *
 * Turn on and off the motor safety option for this relay object.
 *
 * @param enabled True if motor safety is enforced for this object
 */
void Relay::SetSafetyEnabled(bool enabled) {
  m_safetyHelper->SetSafetyEnabled(enabled);
}

/**
 * Check if motor safety is enabled for this object.
 *
 * @returns True if motor safety is enforced for this object
 */
bool Relay::IsSafetyEnabled() const {
  return m_safetyHelper->IsSafetyEnabled();
}

void Relay::GetDescription(std::ostringstream& desc) const {
  desc << "Relay " << GetChannel();
}

void Relay::ValueChanged(ITable* source, llvm::StringRef key,
                         std::shared_ptr<nt::Value> value, bool isNew) {
  if (!value->IsString()) return;
  if (value->GetString() == "Off")
    Set(kOff);
  else if (value->GetString() == "Forward")
    Set(kForward);
  else if (value->GetString() == "Reverse")
    Set(kReverse);
  else if (value->GetString() == "On")
    Set(kOn);
}

void Relay::UpdateTable() {
  if (m_table != nullptr) {
    if (Get() == kOn) {
      m_table->PutString("Value", "On");
    } else if (Get() == kForward) {
      m_table->PutString("Value", "Forward");
    } else if (Get() == kReverse) {
      m_table->PutString("Value", "Reverse");
    } else {
      m_table->PutString("Value", "Off");
    }
  }
}

void Relay::StartLiveWindowMode() {
  if (m_table != nullptr) {
    m_table->AddTableListener("Value", this, true);
  }
}

void Relay::StopLiveWindowMode() {
  if (m_table != nullptr) {
    m_table->RemoveTableListener(this);
  }
}

std::string Relay::GetSmartDashboardType() const { return "Relay"; }

void Relay::InitTable(std::shared_ptr<ITable> subTable) {
  m_table = subTable;
  UpdateTable();
}

std::shared_ptr<ITable> Relay::GetTable() const { return m_table; }
