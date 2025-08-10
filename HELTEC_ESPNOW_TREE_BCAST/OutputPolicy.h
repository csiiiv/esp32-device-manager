#ifndef OUTPUT_POLICY_H
#define OUTPUT_POLICY_H

#include "DataManager.h"  // For DistributedIOData, MAX_INPUTS, etc.

namespace OutputPolicy {

/**
 * computeOutputsFromInputs
 *
 * Central place to define network-wide output behavior (Q) from inputs (I).
 * Children do not compute outputs; they simply apply Q for their own bit index.
 *
 * Edit this function to change output logic. The shipped default is pass-through.
 */
void computeOutputsFromInputs(DistributedIOData& ioFrame);

// Helpers to read individual bit states from the I/Q frames (zero-based indices)
// bitIndex: 0..31, inputIndex/outputIndex: 0..2 (I0..I2, Q0..Q2)
bool getInputBit(const DistributedIOData& ioFrame, int bitIndex /*0-31*/, int inputIndex /*0-2*/);
bool getOutputBit(const DistributedIOData& ioFrame, int bitIndex /*0-31*/, int outputIndex /*0-2*/);

// Helpers to set individual bit states in the I/Q frames (zero-based indices)
void setInputBit(DistributedIOData& ioFrame, int bitIndex /*0-31*/, int inputIndex /*0-2*/, bool value);
void setOutputBit(DistributedIOData& ioFrame, int bitIndex /*0-31*/, int outputIndex /*0-2*/, bool value);

} // namespace OutputPolicy

#endif // OUTPUT_POLICY_H


