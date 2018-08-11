#pragma once

#include "compat.h"

#include "SoundMixdownEffect.h"
#include "FilterPosition.h"

#define POSITIONAL_EFFECT_ISLOCAL    0x00000000
#define POSITIONAL_EFFECT_ISWORLD    0x00000001

class CPositionalMixdownEffect : public CSoundMixdownEffect {
private:
    static CFilterPosition m_positionFilter;
public:
    CPositionalMixdownEffect() {
        this->setParam("isWorld", POSITIONAL_EFFECT_ISWORLD);
        this->setParam("isLoudSpeaker", 0.0f);
        this->setParam("speakerPosX", 0.0f);
        this->setParam("speakerPosY", 0.0f);
        this->setParam("speakerPosZ", 0.0f);
        this->setParam("headVectorX", 0.0f);
        this->setParam("headVectorY", 1.0f);
        this->setParam("headVectorZ", 0.0f);
        this->setParam("curveScale", 1.0f);
        this->setParam("speakingType", ACRE_SPEAKING_DIRECT);
    };
    void process(int16_t *const a_samples, int32_t ac_sampleCount, int32_t ac_channels, const uint32_t ac_speakerMask) {
        this->m_positionFilter.process(a_samples, ac_sampleCount, ac_channels, ac_speakerMask, this);
    };
};
