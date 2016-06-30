/*----------------------------------------------------------------------------*/
/* Copyright (c) FIRST 2016. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include <assert.h>
#include <jni.h>
#include "Log.h"

#include "edu_wpi_first_wpilibj_hal_RelayJNI.h"

#include "HAL/Relay.h"
#include "HALUtil.h"

// set the logging level
TLogLevel relayJNILogLevel = logWARNING;

#define RELAYJNI_LOG(level)     \
  if (level > relayJNILogLevel) \
    ;                           \
  else                          \
  Log().Get(level)

extern "C" {

/*
 * Class:     edu_wpi_first_wpilibj_hal_RelayJNI
 * Method:    initializeRelayPort
 * Signature: (IZ)I;
 */
JNIEXPORT jint JNICALL Java_edu_wpi_first_wpilibj_hal_RelayJNI_initializeRelayPort(
    JNIEnv* env, jclass, jint id, jboolean fwd) {
  RELAYJNI_LOG(logDEBUG) << "Calling RELAYJNI initializeRelayPort";
  RELAYJNI_LOG(logDEBUG) << "Port Handle = " << (HalPortHandle)id;
  RELAYJNI_LOG(logDEBUG) << "Forward = " << (jint)fwd;
  int32_t status = 0;
  HalRelayHandle handle = initializeRelayPort((HalPortHandle)id, (uint8_t) fwd, &status);
  RELAYJNI_LOG(logDEBUG) << "Status = " << status;
  RELAYJNI_LOG(logDEBUG) << "Relay Handle = " << handle;
  return (jint) handle;
}

/*
* Class:     edu_wpi_first_wpilibj_hal_RelayJNI
* Method:    freeRelayPort
* Signature: (I)V;
*/
JNIEXPORT void JNICALL Java_edu_wpi_first_wpilibj_hal_RelayJNI_freeRelayPort(
    JNIEnv *env, jclass, jint id) {
  RELAYJNI_LOG(logDEBUG) << "Calling RELAYJNI freeRelayPort";
  RELAYJNI_LOG(logDEBUG) << "Port Handle = " << (HalRelayHandle)id;
  freeRelayPort((HalRelayHandle)id);
}

/*
* Class:     edu_wpi_first_wpilibj_hal_RelayJNI
* Method:    checkRelayChannel
* Signature: (I)Z;
*/
JNIEXPORT jboolean JNICALL Java_edu_wpi_first_wpilibj_hal_RelayJNI_checkRelayChannel(
    JNIEnv *env, jclass, jint channel) {
  RELAYJNI_LOG(logDEBUG) << "Calling RELAYJNI checkRelayChannel";
  RELAYJNI_LOG(logDEBUG) << "Channel = " << channel;
  return (jboolean)checkRelayChannel((uint8_t) channel);
}

/*
 * Class:     edu_wpi_first_wpilibj_hal_RelayJNI
 * Method:    setRelay
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_edu_wpi_first_wpilibj_hal_RelayJNI_setRelay(
    JNIEnv* env, jclass, jint id, jboolean value) {
  RELAYJNI_LOG(logDEBUG) << "Calling RELAYJNI setRelay";
  RELAYJNI_LOG(logDEBUG) << "Port Handle = " << (HalRelayHandle)id;
  RELAYJNI_LOG(logDEBUG) << "Flag = " << (jint)value;
  int32_t status = 0;
  setRelay((HalRelayHandle)id, value, &status);
  RELAYJNI_LOG(logDEBUG) << "Status = " << status;
  CheckStatus(env, status);
}

/*
 * Class:     edu_wpi_first_wpilibj_hal_RelayJNI
 * Method:    getRelay
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_edu_wpi_first_wpilibj_hal_RelayJNI_getRelay(
    JNIEnv* env, jclass, jint id) {
  RELAYJNI_LOG(logDEBUG) << "Calling RELAYJNI getRelay";
  RELAYJNI_LOG(logDEBUG) << "Port Handle = " << (HalRelayHandle)id;
  int32_t status = 0;
  jboolean returnValue = getRelay((HalRelayHandle)id, &status);
  RELAYJNI_LOG(logDEBUG) << "Status = " << status;
  RELAYJNI_LOG(logDEBUG) << "getRelayResult = " << (jint)returnValue;
  CheckStatus(env, status);
  return returnValue;
}

}  // extern "C"
