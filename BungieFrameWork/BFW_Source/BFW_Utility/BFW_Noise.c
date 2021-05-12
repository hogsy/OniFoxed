/* noise.c : Filtered pseudorandom noise functions.
** 
** This file defines the functions to generate filtered random noise
** in one, two and three dimensions (so far).
**
** Based reasonably closely on Geoff Wyvill's pseudorandom filtered noise.
**
** Chris Butcher, 9 December 1996
*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Noise.h"

// Constants that determine the parameters of the pseudo-random number
// sequence used for generation of noise values.
#define BASE_PARAM1       (5 * 5 * 5 * 5 * 5 * 5)
#define BASE_PARAM2       49

// The distance after which noise will repeat itself (necessary because
// we are mapping xyz-space into one dimension).
static UUtUns32 noise_extent, noise_extent_sq;

// Parameters to let us shift a row or layer in noise space directly rather
// than performing a large number of column shifts. Precalculated by the
// call to initialise_noise().
static UUtUns32 rowshift_param1, rowshift_param2;
static UUtUns32 layershift_param1, layershift_param2;

// Prototype for the noise random number generator. Used locally only.
static UUtUns32 generator(UUtUns32 seed);

// Macros to shift the random number generator over a column, row or layer.
#define ADVANCE_COLUMN  x -= 1.0f;                                         \
                        random_accum = random_accum * BASE_PARAM1         \
                                                    + BASE_PARAM2;  

#define ADVANCE_ROW     y -= 1.0f;                                         \
                        x += 3.0f;                                         \
                        random_accum = random_accum * rowshift_param1     \
                                                    + rowshift_param2;  

#define ADVANCE_LAYER   z -= 1.0f;                                         \
                        y += 3.0f;                                         \
                        x += 3.0f;                                         \
                        random_accum = random_accum * layershift_param1   \
                                                    + layershift_param2;  

// The cubic filter that is used in 1D to interpolate the pseudo-random
// noise values.
#define CUBIC_1D_FILTER   cubic = x*x;                                      \
                          if (cubic < 4.0f) {                                \
                            cubic = 1 - (0.25f * cubic);                     \
                            cubic *= cubic*cubic;                           \
                            sum += cubic * random_accum * 4294967296.f /* S.S. / (65536.0f*65536.0f)*/;\
                            weightsum += cubic;                             \
                          }

// The differential filter that is used in 1D to interpolate the pseudo-random
// noise values and find the derivative..
#define CUBIC_1D_DERIV    cubic = x*x;                                      \
                          if (cubic < 4.0f) {                                \
                            random_val = random_accum * 4294967296.f /* S.S. / (65536.0f*65536.0f)*/;  \
                            cubic = 1 - (0.25f * cubic);                     \
                            cubic *= cubic;                                 \
			    deriv = -1.5f * cubic;                           \
                            deriv_x_weight += deriv * x;                    \
                            deriv *= random_val;                            \
                            deriv_x_sum += deriv * x;                       \
                            cubic_sum += cubic * random_val;                \
                            cubic_weightsum += cubic;                       \
                          }

// The cubic filter that is used in 2D to interpolate the pseudo-random
// noise values.
#define CUBIC_2D_FILTER   cubic = x*x + y*y;                                \
                          if (cubic < 4.0f) {                                \
                            cubic = 1 - (0.25f * cubic);                     \
                            cubic *= cubic*cubic;                           \
                            sum += cubic * random_accum * 4294967296.f /* S.S. / (65536.0f*65536.0f)*/;\
                            weightsum += cubic;                             \
                          }

// The differential filter that is used in 2D to interpolate the pseudo-random
// noise values and find the derivative..
#define CUBIC_2D_DERIV    cubic = x*x + y*y;                                \
                          if (cubic < 4.0f) {                                \
                            random_val = random_accum * 4294967296.f /* S.S. / (65536.0f*65536.0f)*/;  \
                            cubic = 1 - (0.25f * cubic);                     \
                            cubic *= cubic;                                 \
			    deriv = -1.5f * cubic;                           \
                            deriv_x_weight += deriv * x;                    \
			    deriv_y_weight += deriv * y;                    \
                            deriv *= random_val;                            \
                            deriv_x_sum += deriv * x;                       \
                            deriv_y_sum += deriv * y;                       \
                            cubic_sum += cubic * random_val;                \
                            cubic_weightsum += cubic;                       \
                          }

// The cubic filter that is used in 3D to interpolate the pseudo-random
// noise values.
#define CUBIC_3D_FILTER   cubic = x*x + y*y + z*z;                          \
                          if (cubic < 4.0f) {                                \
                            cubic = 1 - (0.25f * cubic);                     \
                            cubic *= cubic*cubic;                           \
                            sum += cubic * random_accum * 4294967296.f /* S.S. / (65536.0f*65536.0f)*/;\
                            weightsum += cubic;                             \
                          }

// The differential filter that is used in 2D to interpolate the pseudo-random
// noise values and find the derivative..
#define CUBIC_3D_DERIV    cubic = x*x + y*y + z*z;                          \
                          if (cubic < 4.0f) {                                \
                            random_val = random_accum * 4294967296.f /* S.S. / (65536.0f*65536.0f)*/;  \
                            cubic = 1 - (0.25f * cubic);                     \
                            cubic *= cubic;                                 \
			    deriv = -1.5f * cubic;                           \
                            deriv_x_weight += deriv * x;                    \
			    deriv_y_weight += deriv * y;                    \
			    deriv_z_weight += deriv * z;                    \
                            deriv *= random_val;                            \
                            deriv_x_sum += deriv * x;                       \
                            deriv_y_sum += deriv * y;                       \
                            deriv_z_sum += deriv * z;                       \
                            cubic_sum += cubic * random_val;                \
                            cubic_weightsum += cubic;                       \
                          }


// The actual random number generator. Passed an UUtUns32 as a seed
// for the random number, returns a pseudo-random UUtUns32.
UUtUns32 generator(UUtUns32 seed)
{
  // The parameters that determine the congruency sequence which is
  // applied to generate a pseudorandom number.
  UUtUns32 param1, param2;

  // The accumulator to use for the congruency sequence.
  UUtUns32 accum;

  // Generate a pseudorandom number using the congruency sequence that
  // Geoff coded.
  accum = 1;
  param1 = BASE_PARAM1;
  param2 = BASE_PARAM2;
  for ( ; seed > 0 ; seed >>= 1) {
    if (seed & 1)
      accum = accum * param1 + param2;

    param2 += param1 * param2;
    param1 *= param1;
  }

  // Return the end result of the congruency sequence.
  return accum;
}

//   We want to be able to index the random number generator in three
// dimensions. We do this by indexing it with the number
// seed = (z*z_scale + y*y_scale + x) where y_scale = noise_extent and
// z_scale = SQR(noise_extent).
//   We have defined a pseudo-random number generator for one dimension.
// Since moving one unit in Z or Y is equivalent to moving some large
// number of units in X, we can pre-calculate parameters that describe
// how the new random number can be calculated after a layer or row shift.
void UUrNoise_Initialize(UUtUns32 extent)
{
	UUtUns32 dist, param1, param2;

	noise_extent = extent;
	noise_extent_sq = extent*extent;

	// Determine the row-shift parameters.
	rowshift_param1 = 1;
	rowshift_param2 = 0;
	param1 = BASE_PARAM1;
	param2 = BASE_PARAM2;
	for (dist = noise_extent - 3; dist > 0; dist >>= 1) 
	{
		if (dist & 1) 
		{
			rowshift_param2 += rowshift_param1 * param2;
			rowshift_param1 *= param1;
		}
		param2 += param1 * param2;
		param1 *= param1;
	}

	// Determine the layer-shift parameters. Note that this is not for a shift
	// by one in Z only, but also by -3 in Y and -3 in X.
	layershift_param1 = 1;
	layershift_param2 = 0;
	param1 = BASE_PARAM1;
	param2 = BASE_PARAM2;
	for (dist = noise_extent_sq - 3*noise_extent - 3; dist > 0; dist >>= 1) 
	{
		if (dist & 1) 
		{
			layershift_param2 += layershift_param1 * param2;
			layershift_param1 *= param1;
		}
		param2 += param1 * param2;
		param1 *= param1;
	}
}

// One-dimensional interpolation of the pseudorandom numbers produced
// by generator(). Uses the cubic filter (1 - t)^3 to interpolate.
float UUrNoise_1(float samplept)
{
  // The random accumulator.
  UUtUns32 random_accum;

  UUtInt32 x_pt;
  float x, sum, weightsum, cubic;

  sum = 0.0;
  weightsum = 0.0;

  x_pt = (UUtInt32) floor(samplept) - 1;

  x = samplept - x_pt;

  // Index the 1D random number space based on the input number.
  random_accum = generator(x_pt);

  CUBIC_1D_FILTER          ADVANCE_COLUMN
  CUBIC_1D_FILTER          ADVANCE_COLUMN
  CUBIC_1D_FILTER          ADVANCE_COLUMN
  CUBIC_1D_FILTER

  return (sum / weightsum);
}

// One-dimensional interpolation of the pseudorandom numbers produced
// by generator(). Uses the cubic filter (1 - t)^3 to interpolate, and
// also determines the gradient of the noise..
static float noise_1grad(float samplept, float *gradient)
{
  // The random accumulator.
  UUtUns32 random_accum;

  UUtInt32 x_pt;
  float x;
  float cubic_sum, cubic_weightsum, cubic;
  float deriv_x_sum, deriv_x_weight, deriv;
  float random_val;

  cubic_sum = 0.0;
  cubic_weightsum = 0.0;
  deriv_x_sum = 0.0;
  deriv_x_weight = 0.0;

  x_pt = (UUtInt32) floor(samplept) - 1;

  x = samplept - x_pt;

  // Index the 1D random number space based on the input number.
  random_accum = generator(x_pt);

  CUBIC_1D_DERIV          ADVANCE_COLUMN
  CUBIC_1D_DERIV          ADVANCE_COLUMN
  CUBIC_1D_DERIV          ADVANCE_COLUMN
  CUBIC_1D_DERIV

  // Calculate the gradient from the values that we've accumulated
  // from the noise table.
  *gradient = (cubic_weightsum * deriv_x_sum - cubic_sum * deriv_x_weight)
    / (cubic_weightsum * cubic_weightsum);

  return (cubic_sum / cubic_weightsum);
}

// Two-dimensional interpolation of the pseudorandom numbers produced
// by generator(). Uses the cubic filter (1 - t)^3 to interpolate.
static float UUrNoise_2(float *samplept)
{
  // The random accumulator.
  UUtUns32 random_accum;

  UUtInt32 x_pt, y_pt;
  float x, y, sum, weightsum, cubic;

  sum = 0.0;
  weightsum = 0.0;

  x_pt = (UUtInt32) floor(samplept[0]) - 1;
  y_pt = (UUtInt32) floor(samplept[1]) - 1;

  x = samplept[0] - x_pt;
  y = samplept[1] - y_pt;

  // Index the 1D random number space based on the input vector.
  random_accum = generator(y_pt*noise_extent + x_pt);

  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_ROW

  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_ROW

  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_ROW

  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER          ADVANCE_COLUMN
  CUBIC_2D_FILTER

  return (sum / weightsum);
}


// Two-dimensional interpolation of the pseudorandom numbers produced
// by generator(). Uses the cubic filter (1 - t)^3 to interpolate, and
// also determines the gradient of the noise in 2D.
static float noise_2grad(float *samplept, float *gradient)
{
  // The random accumulator.
  UUtUns32 random_accum;

  UUtInt32 x_pt, y_pt;
  float x, y, cubic_sum, cubic_weightsum, cubic, deriv;
  float deriv_x_sum, deriv_x_weight;
  float deriv_y_sum, deriv_y_weight;
  float random_val;

  cubic_sum = 0.0;
  cubic_weightsum = 0.0;
  deriv_x_sum = 0.0;
  deriv_x_weight = 0.0;
  deriv_y_sum = 0.0;
  deriv_y_weight = 0.0;

  x_pt = (UUtInt32) floor(samplept[0]) - 1;
  y_pt = (UUtInt32) floor(samplept[1]) - 1;

  x = samplept[0] - x_pt;
  y = samplept[1] - y_pt;

  // Index the 1D random number space based on the input vector.
  random_accum = generator(y_pt*noise_extent + x_pt);

  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_ROW

  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_ROW

  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_ROW

  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV          ADVANCE_COLUMN
  CUBIC_2D_DERIV

  // Calculate the gradient from the values that we've accumulated
  // from the noise table.
  gradient[0] = (cubic_weightsum * deriv_x_sum - cubic_sum * deriv_x_weight)
    / (cubic_weightsum * cubic_weightsum);
  gradient[1] = (cubic_weightsum * deriv_y_sum - cubic_sum * deriv_y_weight)
    / (cubic_weightsum * cubic_weightsum);

  return (cubic_sum / cubic_weightsum);
}


// Three-dimensional interpolation of the pseudorandom numbers produced
// by generator(). Uses the cubic filter (1 - t)^3 to interpolate.
static float UUrNoise_3(float *samplept)
{
  // The random accumulator.
  UUtUns32 random_accum;

  UUtInt32 x_pt, y_pt, z_pt;
  float x, y, z, sum, weightsum, cubic;

  sum = 0.0;
  weightsum = 0.0;

  x_pt = (UUtInt32) floor(samplept[0]) - 1;
  y_pt = (UUtInt32) floor(samplept[1]) - 1;
  z_pt = (UUtInt32) floor(samplept[2]) - 1;

  x = samplept[0] - x_pt;
  y = samplept[1] - y_pt;
  z = samplept[2] - z_pt;

  // Index the 1D random number space based on the input vector.
  random_accum = generator(z_pt*noise_extent_sq + y_pt*noise_extent + x_pt);

  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_LAYER

  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_LAYER

  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_LAYER

  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_ROW
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER          ADVANCE_COLUMN
  CUBIC_3D_FILTER

  return sum / weightsum;
}

static float noise_3grad(float *samplept, float *gradient)
{
  // The random accumulator.
  UUtUns32 random_accum;

  UUtInt32 x_pt, y_pt, z_pt;
  float x, y, z, cubic_sum, cubic_weightsum, cubic, deriv;
  float deriv_x_sum, deriv_x_weight;
  float deriv_y_sum, deriv_y_weight;
  float deriv_z_sum, deriv_z_weight;
  float random_val;

  cubic_sum = 0.0;
  cubic_weightsum = 0.0;
  deriv_x_sum = 0.0;
  deriv_x_weight = 0.0;
  deriv_y_sum = 0.0;
  deriv_y_weight = 0.0;
  deriv_z_sum = 0.0;
  deriv_z_weight = 0.0;

  x_pt = (UUtInt32) floor(samplept[0]) - 1;
  y_pt = (UUtInt32) floor(samplept[1]) - 1;
  z_pt = (UUtInt32) floor(samplept[2]) - 1;

  x = samplept[0] - x_pt;
  y = samplept[1] - y_pt;
  z = samplept[2] - z_pt;

  // Index the 1D random number space based on the input vector.
  random_accum = generator(z_pt*noise_extent_sq + y_pt*noise_extent + x_pt);

  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_LAYER

  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_LAYER

  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_LAYER

  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_ROW
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV          ADVANCE_COLUMN
  CUBIC_3D_DERIV

  // Calculate the gradient from the values that we've accumulated
  // from the noise table.
  gradient[0] = (cubic_weightsum * deriv_x_sum - cubic_sum * deriv_x_weight)
    / (cubic_weightsum * cubic_weightsum);
  gradient[1] = (cubic_weightsum * deriv_y_sum - cubic_sum * deriv_y_weight)
    / (cubic_weightsum * cubic_weightsum);
  gradient[2] = (cubic_weightsum * deriv_z_sum - cubic_sum * deriv_z_weight)
    / (cubic_weightsum * cubic_weightsum);

  return (cubic_sum / cubic_weightsum);
}

static void vector_noise3(float *invect, float *outvect)
{
  float sample1[3], sample2[3], sample3[3];

  sample1[0] = invect[0] + noise_extent/2;
  sample1[1] = invect[1];
  sample1[2] = invect[2];
  sample2[0] = invect[0];
  sample2[1] = invect[1] + noise_extent/2;
  sample2[2] = invect[2];
  sample3[0] = invect[0];
  sample3[1] = invect[1];
  sample3[2] = invect[2] + noise_extent/2;

  outvect[0] = UUrNoise_3(sample1);
  outvect[1] = UUrNoise_3(sample2);
  outvect[2] = UUrNoise_3(sample3);
}

