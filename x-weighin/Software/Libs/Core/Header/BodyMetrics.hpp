/**
 ******************************************************************************
 * @file    BodyMetrics.hpp
 * @brief   BMI and resting energy from the weight, the UNA profile's height,
 *          and the age and sex from the phone settings.
 *
 * Integers only: no floating point, the same answer on the watch and the host.
 * Both are meant to take the trend weight, which moves slowly, rather than one
 * day's reading.
 ******************************************************************************
 */

#ifndef XWEIGHIN_BODY_METRICS_HPP
#define XWEIGHIN_BODY_METRICS_HPP

#include <cstdint>

namespace XWeighIn
{

/// Heights outside this range (cm) are treated as not set rather than used.
constexpr uint32_t kMinHeightCm = 50;
constexpr uint32_t kMaxHeightCm = 272;

/**
 * @brief   Body mass index, kg / m^2, in tenths (247 is 24.7), rounded.
 * @return  0 if the weight or height is not set.
 */
int32_t bmiTenths(int32_t hundredths, uint32_t heightCm);

/**
 * @brief   Resting energy (basal metabolic rate), kcal per day, by the
 *          Mifflin-St Jeor equation (Mifflin MD, St Jeor ST et al., "A new
 *          predictive equation for resting energy expenditure in healthy
 *          individuals", Am J Clin Nutr 1990;51:241-7):
 *
 *              10 x kg + 6.25 x cm - 5 x age + 5     (men)
 *              10 x kg + 6.25 x cm - 5 x age - 161   (women)
 *
 *          rounded to the nearest kcal.
 *
 * @param   sex  The config field: 1 female, 2 male.
 * @return  0 if the weight, height, age or sex is not set.
 */
int32_t bmrKcal(int32_t hundredths, uint32_t heightCm, uint8_t ageYears, uint8_t sex);

}  // namespace XWeighIn

#endif  // XWEIGHIN_BODY_METRICS_HPP
