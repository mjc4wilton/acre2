#pragma once

#include "compat.h"
#include "Types.h"
#include "Macros.h"

#include "AcreDsp.h"

class CFilterOcclusion
{
public:
    CFilterOcclusion(void);
    ~CFilterOcclusion(void);
    ACRE_RESULT process(int16_t *const a_samples, const int32_t ac_sampleCount, const int32_t ac_channels, const ACRE_VOLUME ac_volume, Dsp::Filter *&a_filter);

    virtual __inline void setChannelCount(const int32_t ac_value) { this->m_channelCount = ac_value; }
    virtual __inline int32_t getChannelCount() const { return this->m_channelCount; }

protected:
    int32_t m_channelCount;
};
