#include "vib_detect.h"
#include "app_config.h"

static float safe_ratio(float value, float base)
{
    return value / (base + 0.0001f);
}

static float positive_delta(float ratio)
{
    return ratio > 1.0f ? (ratio - 1.0f) : 0.0f;
}

void VibDetect_Init(VibDetector *det)
{
    det->learn_count = 0U;
    det->base_rms = 0.0f;
    det->base_p2p = 0.0f;
    det->base_crest = 0.0f;
    det->bad_count = 0U;
    det->good_count = 0U;
    det->state = VIB_STATE_LEARNING;
}

VibResult VibDetect_Update(VibDetector *det, const VibFeature *feature, uint32_t seq, uint32_t dropped)
{
    VibResult result;
    float rms_ratio;
    float p2p_ratio;
    float crest_ratio;
    float score_f;

    result.seq = seq;
    result.feature = *feature;
    result.dropped_windows = dropped;

    if (det->learn_count < VIB_LEARN_WINDOWS) {
        det->learn_count++;
        det->base_rms += (feature->rms - det->base_rms) / (float)det->learn_count;
        det->base_p2p += (feature->p2p - det->base_p2p) / (float)det->learn_count;
        det->base_crest += (feature->crest - det->base_crest) / (float)det->learn_count;
        det->state = VIB_STATE_LEARNING;
        result.state = det->state;
        result.score = (uint8_t)(det->learn_count * 100U / VIB_LEARN_WINDOWS);
        return result;
    }

    rms_ratio = safe_ratio(feature->rms, det->base_rms);
    p2p_ratio = safe_ratio(feature->p2p, det->base_p2p);
    crest_ratio = safe_ratio(feature->crest, det->base_crest);

    /*
     * Only the part above the learned baseline should contribute to score.
     * If ratio is around 1.0, the machine is close to normal and score stays low.
     */
    score_f = 45.0f * positive_delta(rms_ratio) +
              40.0f * positive_delta(p2p_ratio) +
              15.0f * positive_delta(crest_ratio);
    if (score_f > 100.0f) {
        score_f = 100.0f;
    }
    if (score_f < 0.0f) {
        score_f = 0.0f;
    }
    result.score = (uint8_t)score_f;

    if (result.score >= VIB_ALARM_SCORE) {
        if (det->bad_count < 255U) det->bad_count++;
        det->good_count = 0U;
    } else {
        if (det->good_count < 255U) det->good_count++;
        if (det->bad_count > 0U) det->bad_count--;
    }

    if (det->bad_count >= VIB_ALARM_HOLD_WINDOWS) {
        det->state = VIB_STATE_ALARM;
    } else if (result.score >= VIB_WARNING_SCORE) {
        det->state = VIB_STATE_WARNING;
    } else {
        det->state = VIB_STATE_NORMAL;
    }

    if (det->good_count >= VIB_RECOVER_HOLD_WINDOWS) {
        det->bad_count = 0U;
        if (det->state != VIB_STATE_SENSOR_ERROR) {
            det->state = VIB_STATE_NORMAL;
        }
    }

    result.state = det->state;
    return result;
}

const char *VibDetect_StateText(VibState state)
{
    switch (state) {
    case VIB_STATE_LEARNING: return "LEARNING";
    case VIB_STATE_NORMAL: return "NORMAL";
    case VIB_STATE_WARNING: return "WARNING";
    case VIB_STATE_ALARM: return "ALARM";
    case VIB_STATE_SENSOR_ERROR: return "SENSOR_ERROR";
    default: return "UNKNOWN";
    }
}
