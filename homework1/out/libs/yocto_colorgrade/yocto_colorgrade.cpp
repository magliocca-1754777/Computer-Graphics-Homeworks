//
// Implementation for Yocto/Grade.
//

//
// LICENSE:
//
// Copyright (c) 2020 -- 2020 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include "yocto_colorgrade.h"

#include <yocto/yocto_color.h>
#include <yocto/yocto_sampling.h>

// -----------------------------------------------------------------------------
// COLOR GRADING FUNCTIONS
// -----------------------------------------------------------------------------
namespace yocto {
double fract(double x) { return x - trunc(x); }
vec3f  stepnoise(vec3f p, float size) {
  p += 10.0;
  float x = floor(p.x / size) * size;
  float y = floor(p.y / size) * size;

  x = fract(x * 0.1) + 1.0 + x * 0.0002;
  y = fract(y * 0.1) + 1.0 + y * 0.0003;

  float a = fract(1.0 / (0.000001 * x * y + 0.00001));
  a       = fract(1.0 / (0.000001234 * a + 0.00001));

  float b = fract(1.0 / (0.000002 * (x * y + x) + 0.00001));
  b       = fract(1.0 / (0.0000235 * b + 0.00001));

  return vec3f{a, b};
}
float tent(float f) { return 1.0 - abs((float)(fract(f) - 0.5)) * 2.0; }

#define SEED1 -0.5775604999999985
#define SEED2 6.440483302499992

float mask(vec3f p) {
  vec3f r = stepnoise(p, 8.423424);
  p[0] += r[0];
  p[1] += r[1];

  float f1 = tent(p[0] * SEED1 + p[1] / (SEED1 + 0.5));
  float f2 = tent(p[1] * SEED2 + p[0] / (SEED2 + 0.5));
  float f  = f1 * f2;

  // f = pow(f, 4.0)*1.4 + f*0.2;
  f = sqrt(f);
  return f;
}

image<vec4f> grade_image(const image<vec4f>& img, const grade_params& params) {
  // PUT YOUR CODE HERE
  auto graded = img;
  // tone mapping
  for (auto i = 0; i < img.imsize()[0]; i++) {
    for (auto j = 0; j < img.imsize()[1]; j++) {
      auto c = xyz(graded[{i, j}]);
      // exposure compensation
      c = c * pow(2, params.exposure);
      // filmic correction
      if (params.filmic) {
        c *= 0.6;
        c = (pow(c, 2) * 2.51 + c * 0.03) /
            (pow(c, 2) * 2.43 + c * 0.510 + 0.14);
      }
      // srgb color space
      if (params.srgb) c = pow(c, 1 / 2.2);
      // clamp result
      c              = clamp(c, 0.0, 1.0);
      graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
    }
  }
  // color tint
  for (auto i = 0; i < img.imsize()[0]; i++) {
    for (auto j = 0; j < img.imsize()[1]; j++) {
      auto c         = xyz(graded[{i, j}]);
      c              =  operator*(c, params.tint);
      graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
    }
  }

  // saturation
  if (params.saturation || params.saturation == 0) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c         = xyz(graded[{i, j}]);
        auto g         = (c.x + c.y + c.z) / 3;
        c              = g + (c - g) * (params.saturation * 2);
        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  // contrast
  if (params.contrast) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c         = xyz(graded[{i, j}]);
        c              = gain(c, 1 - params.contrast);
        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  // vignette
  if (params.vignette) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto  c    = xyz(graded[{i, j}]);
        auto  vr   = 1 - params.vignette;
        vec2f ij   = {i, j};
        vec2f imgf = {img.imsize()[0], img.imsize()[1]};
        auto  r    = length(operator-(ij, operator/(imgf, 2))) /
                 length(operator/(imgf, 2));
        c              = c * (1 - smoothstep(vr, 2 * vr, r));
        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  // film grain
  if (params.grain) {
    rng_state r = make_rng(1102104);
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c         = xyz(graded[{i, j}]);
        c              = c + (rand1f(r) - 0.5) * params.grain;
        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  // mosaic effect
  if (params.mosaic) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c = xyz(graded[{i - i % params.mosaic, j - j % params.mosaic}]);
        auto d = xyz(graded[{i, j}]);
        d      = c;
        graded[{i, j}] = vec4f{d.x, d.y, d.z, graded[{i, j}].w};
      }
    }
  }

  // grid effect
  if (params.grid) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c = xyz(graded[{i, j}]);
        c      = (i % params.grid == 0 || 0 == j % params.grid) ? 0.5 * c : c;
        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  // fumetto
  if (params.pn) {
    for (auto i = 0; i < img.imsize()[0]; i = i + 15) {
      for (auto j = 0; j < img.imsize()[1]; j = j + 15) {
        int in = j;

        for (auto k = i + 1; k < i + 4; k++) {
          auto c          = xyz(graded[{k, in}]);
          graded[{k, in}] = {1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
        }
        in++;
        for (auto k = i; k < i + 5; k++) {
          auto c          = xyz(graded[{k, in}]);
          graded[{k, in}] = {1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
        }
        in++;
        for (auto k = i - 1; k < i + 6; k++) {
          auto c          = xyz(graded[{k, in}]);
          graded[{k, in}] = {1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
        }
        in++;
        for (auto k = i - 1; k < i + 6; k++) {
          auto c          = xyz(graded[{k, in}]);
          graded[{k, in}] = {1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
        }
        in++;
        for (auto k = i; k < i + 5; k++) {
          auto c          = xyz(graded[{k, in}]);
          graded[{k, in}] = {1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
        }
        in++;
        for (auto k = i + 1; k < i + 4; k++) {
          auto c          = xyz(graded[{k, in}]);
          graded[{k, in}] = {1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
        }
      }
    }
  }
  // red green blue
  if (params.rgb) {
    vec2f inters;
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c = xyz(graded[{i, j}]);

        if (i == j && i == 1920 - j) {
          inters = {(float)i, (float)j};
          for (int k = inters.y; k < img.imsize()[1]; k++) {
            graded[{i, k}] = {0.0, 0.0, 0.0};
          }
        }
        if ((i == j && inters.x == 0) || (i == 1920 - j && inters.x != 0))
          c = {0.0, 0.0, 0.0};
        if (i < j && inters.x == 0) {
          c.x = c.x;
          c.y = c.x;
          c.z = (c.x + 0.93f) / 2;
        } else if (i < 1920 - j) {
          c.x = c.x;
          c.y = (c.y + 0.93f) / 2;
          c.z = c.z;
        } else {
          c.x = (c.x + 0.93f) / 2;
          c.y = c.y;
          c.z = c.z;
        }
        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  // negative
  if (params.negative) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c         = xyz(graded[{i, j}]);
        graded[{i, j}] = vec4f{1 - c.x, 1 - c.y, 1 - c.z, graded[{i, j}].w};
      }
    }
  }
  // vintage
  if (params.vintage) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c         = xyz(graded[{i, j}]);
        graded[{i, j}] = vec4f{c.x, c.y, (0.15f + c.z) / 2, graded[{i, j}].w};
      }
    }
  }
  // posterization
  if (params.posterization) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto c   = xyz(graded[{i, j}]);
        auto max = (c.x > c.y) ? 0 : (c.z > c.y) ? 2 : 1;

        c.x = (max == 0) ? c.x : ((float)(int)(c.x * 9)) / 9;
        c.y = (max == 1) ? c.y : (((float)(int)(c.y * 9))) / 9;
        c.z = (max == 2) ? c.z : (((float)(int)(c.z * 9))) / 9;

        graded[{i, j}] = vec4f{c.x, c.y, (c.z > 0) ? c.z : 0, graded[{i, j}].w};
      }
    }
  }
  // viewfinder
  if (params.viewfinder) {
    auto med0   = img.imsize()[0] / 2;
    auto med1   = img.imsize()[1] / 2;
    auto mindim = min(img.imsize()[0], img.imsize()[1]);
    auto md6    = (int)mindim / 4;

    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto  c    = xyz(graded[{i, j}]);
        auto  vr   = 1 - params.viewfinder;
        vec2f ij   = {i, j};
        vec2f imgf = {img.imsize()[0], img.imsize()[1]};
        auto  r1   = length(operator-=(ij, operator/=(imgf, 2)));
        auto  r2   = length(operator/=(imgf, 2));
        auto  r    = r1 / r2;

        if ((i < med0 - (int)(mindim / 175) ||
                i > med0 + (int)(mindim / 175)) &&
            (j < med1 - (int)(mindim / 175) ||
                j > med1 + (int)(mindim / 175))) {
          c = c * (1 - smoothstep(1.4 * vr, 1.5 * vr, r));

        } else if ((i < med0 + 3 && i > med0 - 3) &&
                   (j < med1 + 3 && j > med1 - 3)) {
          c = {1.0, 0.05, 0.0};

        } else if ((i < med0 + md6 && i > med0 - md6) &&
                   (j < med1 + md6 && j > med1 - md6)) {
          if (i == med0 || j == med1) c = {0.0, 0.0, 0.0};

        } else {
          c = {0.0, 0.0, 0.0};
        }

        graded[{i, j}] = vec4f{c.x, c.y, c.z, graded[{i, j}].w};
      }
    }
  }
  if (params.stippling) {
    for (auto i = 0; i < img.imsize()[0]; i++) {
      for (auto j = 0; j < img.imsize()[1]; j++) {
        auto  uv       = xyz(graded[{i, j}]);
        float c        = mask(uv);
        graded[{i, j}] = vec4f{c, c, c, graded[{i, j}].w};
      }
    }
  }

  return graded;
}

}  // namespace yocto