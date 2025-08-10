#include "OutputPolicy.h"

namespace OutputPolicy {

// Return true if the given input bit is set (zero-based indices)
bool getInputBit(const DistributedIOData& ioFrame, int bitIndex, int inputIndex) {
    if (bitIndex < 0 || bitIndex >= MAX_DISTRIBUTED_IO_BITS) return false;
    if (inputIndex < 0 || inputIndex >= MAX_INPUTS) return false;
    const uint32_t word = ioFrame.sharedData[inputIndex][0];
    const uint32_t mask = (1UL << bitIndex);
    return (word & mask) != 0;
}

// Return true if the given output bit is set (zero-based indices)
bool getOutputBit(const DistributedIOData& ioFrame, int bitIndex, int outputIndex) {
    if (bitIndex < 0 || bitIndex >= MAX_DISTRIBUTED_IO_BITS) return false;
    if (outputIndex < 0 || outputIndex >= MAX_INPUTS) return false;
    const uint32_t word = ioFrame.sharedOutputs[outputIndex][0];
    const uint32_t mask = (1UL << bitIndex);
    return (word & mask) != 0;
}

// Set/clear a specific input bit (zero-based indices)
void setInputBit(DistributedIOData& ioFrame, int bitIndex, int inputIndex, bool value) {
    if (bitIndex < 0 || bitIndex >= MAX_DISTRIBUTED_IO_BITS) return;
    if (inputIndex < 0 || inputIndex >= MAX_INPUTS) return;
    uint32_t word = ioFrame.sharedData[inputIndex][0];
    const uint32_t mask = (1UL << bitIndex);
    if (value) {
        word |= mask;
    } else {
        word &= ~mask;
    }
    ioFrame.sharedData[inputIndex][0] = word;
}

// Set/clear a specific output bit (zero-based indices)
void setOutputBit(DistributedIOData& ioFrame, int bitIndex, int outputIndex, bool value) {
    if (bitIndex < 0 || bitIndex >= MAX_DISTRIBUTED_IO_BITS) return;
    if (outputIndex < 0 || outputIndex >= MAX_INPUTS) return;
    uint32_t word = ioFrame.sharedOutputs[outputIndex][0];
    const uint32_t mask = (1UL << bitIndex);
    if (value) {
        word |= mask;
    } else {
        word &= ~mask;
    }
    ioFrame.sharedOutputs[outputIndex][0] = word;
}

void computeOutputsFromInputs(DistributedIOData& ioFrame) {
    // Start from pass-through for all outputs
    // for (int idx = 0; idx < MAX_INPUTS; idx++) {
    //     ioFrame.sharedOutputs[idx][0] = ioFrame.sharedData[idx][0];
    // }

    // Q0-B0 = I0-B0 && I0-B1 (apply only to bit 0 of Q0)
    const bool b0_i0_state = getInputBit(ioFrame, 0, 0);
    const bool b1_i0_state = getInputBit(ioFrame, 1, 0);
    const bool q0_b0_state = b0_i0_state && b1_i0_state;
    setOutputBit(ioFrame, 0, 0, q0_b0_state);

}

} // namespace OutputPolicy


