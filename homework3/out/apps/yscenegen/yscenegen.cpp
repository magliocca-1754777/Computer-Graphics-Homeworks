//
// LICENSE:
//
// Copyright (c) 2016 -- 2020 Fabio Pellacini
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

#include <yocto/yocto_color.h>
#include <yocto/yocto_commonio.h>
#include <yocto/yocto_geometry.h>
#include <yocto/yocto_image.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
using namespace yocto;

#include <filesystem>
#include <memory>

#include "ext/perlin-noise/noise1234.h"

float noise(const vec3f& p) { return noise3(p.x, p.y, p.z); }
vec2f noise2(const vec3f& p) {
  return {noise(p + vec3f{0, 0, 0}), noise(p + vec3f{3, 7, 11})};
}
vec3f noise3(const vec3f& p) {
  return {noise(p + vec3f{0, 0, 0}), noise(p + vec3f{3, 7, 11}),
      noise(p + vec3f{13, 17, 19})};
}
float fbm(const vec3f& p, int octaves) {
  auto sum    = 0.0f;
  auto weight = 1.0f;
  auto scale  = 1.0f;
  for (auto octave = 0; octave < octaves; octave++) {
    sum += weight * fabs(noise(p * scale));
    weight /= 2;
    scale *= 2;
  }
  return sum;
}
float fract(float k) { return k - floor(k); }

vec3f hash3(vec3f p) {
  vec3f q = vec3f{dot(p, vec3f{127.1, 311.7, 411.3}),
      dot(p, vec3f{269.5, 183.3, 152.4}), dot(p, vec3f{419.2, 371.9, 441.0})};

  return vec3f{fract(sinf(q.x) * (float)43758.5453),
      fract(sinf(q.y) * (float)43758.5453),
      fract(sinf(q.z) * (float)43758.5453)};
}
float rand1(vec3f p) {
  return fract(sinf(dot(p, vec3f{419.2, 371.9, 211.2})) * 833458.57832);
}
float voronoise(vec3f p, float u, float v) {
  vec3f cell       = vec3f{floor(p.x), floor(p.y), floor(p.z)};
  vec3f cellOffset = vec3f{fract(p.x), fract(p.y), fract(p.z)};

  float sharpness = 1.0 + 63.0 * pow(1.0 - v, 4.0);

  float value = 0.0;
  float accum = 0.0;
  for (int z = -2; z <= 2; z++) {
    for (int x = -2; x <= 2; x++) {
      for (int y = -2; y <= 2; y++) {
        vec3f samplePos      = vec3f{(float)y, (float)x, (float)z};
        vec3f center         = hash3(cell + samplePos) * u;
        float centerDistance = length(samplePos - cellOffset + center);

        float sample = pow(
            1.0 - smoothstep(0.0, 1.414, centerDistance), sharpness);

        float color = rand1(cell + samplePos);
        value += color * sample;
        accum += sample;
      }
    }
  }
  return value / accum;
}
float cellnoise(vec3f p) {
  vec3f cell       = vec3f{floor(p.x), floor(p.y), floor(p.z)};
  vec3f cellOffset = vec3f{fract(p.x), fract(p.y), fract(p.z)};

  float value = 0.0;
  float accum = 0.0;
  for (int z = -2; z <= 2; z++) {
    for (int x = -2; x <= 2; x++) {
      for (int y = -2; y <= 2; y++) {
        vec3f samplePos = vec3f{(float)y, (float)x, (float)z};

        float centerDistance = length(samplePos - cellOffset);

        float sample = pow(1.0 - smoothstep(0.0, 1.414, centerDistance), 64.0);

        float color = rand1(cell + samplePos);
        value += color * sample;
        accum += sample;
      }
    }
  }
  return value / accum;
}

float smoothvoronoi(vec3f k) {
  vec3f p   = vec3f{floor(k.x), floor(k.y), floor(k.z)};
  vec3f f   = vec3f{fract(k.x), fract(k.y), fract(k.z)};
  float res = 0.0;

  auto rng = make_rng(193843);
  for (int z = -1; z <= 1; z++) {
    for (int j = -1; j <= 1; j++) {
      for (int i = -1; i <= 1; i++) {
        vec3f b = vec3f{(float)i, (float)j, (float)z};
        vec3f r = b - f + rand1(p + b);
        float d = dot(r, r);
        res += 1.0 / pow(d, 8.0);
      }
    }
  }
  return pow(1.0 / res, 1.0 / 16.0);
}

float voronoiDistance(vec3f x) {
  vec3f     p = vec3f{floor(x.x), floor(x.y), floor(x.z)};
  vec3f     f = vec3f{fract(x.x), fract(x.y), fract(x.z)};
  vec3f     mb;
  vec3f     mr;
  float     res = 8.0;
  rng_state rng = make_rng(193840);
  for (int k = -1; k <= 1; k++) {
    for (int j = -1; j <= 1; j++) {
      for (int i = -1; i <= 1; i++) {
        vec3f b = vec3f{(float)i, (float)j, (float)k};
        vec3f r = b + rand3f(rng) - f;
        float d = dot(r, r);

        if (d < res) {
          res = d;
          mr  = r;
          mb  = b;
        }
      }
    }
  }

  res = 8.0;

  for (int k = -1; k <= 1; k++) {
    for (int j = -2; j <= 2; j++) {
      for (int i = -2; i <= 2; i++) {
        vec3f b = mb + vec3f{(float)i, (float)j, (float)k};
        vec3f r = b + rand3f(rng) - f;

        float d = dot(0.5 * (mr + r), normalize(r - mr));
        res     = min(res, d);
      }
    }
  }
  return res;
}

float getBorder(vec3f p) {
  float d = voronoiDistance(p);
  return 1.0 - smoothstep(0.0, 0.05, d);
}

float poxo(vec3f p) {
  rng_state rng = make_rng(193840);
  vec3f     k   = vec3f{floor(p.x), floor(p.y), floor(p.z)};
  vec3f     f   = vec3f{fract(p.x), fract(p.y), fract(p.z)};
  float     res = 0.0;
  for (int z = -2; z <= 2; z++) {
    for (int j = -2; j <= 2; j++) {
      for (int i = -2; i <= 2; i++) {
        vec3f b = vec3f{(float)i, (float)j, (float)z};
        vec3f r = b - f + rand1(p + b);

        float d = dot(r, r);
        res += 1.0 / pow(d, 16.0);
      }
    }
  }

  return pow(1.0 / res, 1.0 / 16.0);
}

float turbulence(const vec3f& p, int octaves) {
  auto sum    = 0.0f;
  auto weight = 1.0f;
  auto scale  = 1.0f;

  for (auto octave = 0; octave < octaves; octave++) {
    sum += weight * fabs(noise(p * scale));
    weight /= 2;
    scale *= 2;
  }
  return sum;
}

float ridge(const vec3f& p, int octaves) {
  auto sum    = 0.0f;
  auto weight = 0.5f;
  auto scale  = 1.0f;
  for (auto octave = 0; octave < octaves; octave++) {
    sum += weight * (1 - fabs(noise(p * scale))) * (1 - fabs(noise(p * scale)));
    weight /= 2;
    scale *= 2;
  }
  return sum;
}

sceneio_instance* get_instance(sceneio_scene* scene, const string& name) {
  for (auto instance : scene->instances)
    if (instance->name == name) return instance;
  print_fatal("unknown instance " + name);
  return nullptr;
}

void add_polyline(sceneio_shape* shape, const vector<vec3f>& positions,
    const vector<vec4f>& colors, float thickness = 0.0001f) {
  auto offset = (int)shape->positions.size();
  shape->positions.insert(
      shape->positions.end(), positions.begin(), positions.end());
  shape->colors.insert(shape->colors.end(), colors.begin(), colors.end());
  shape->radius.insert(shape->radius.end(), positions.size(), thickness);
  for (auto idx = 0; idx < positions.size() - 1; idx++) {
    shape->lines.push_back({offset + idx, offset + idx + 1});
  }
}

void sample_shape(vector<vec3f>& positions, vector<vec3f>& normals,
    vector<vec2f>& texcoords, sceneio_shape* shape, int num) {
  auto triangles  = shape->triangles;
  auto qtriangles = quads_to_triangles(shape->quads);
  triangles.insert(triangles.end(), qtriangles.begin(), qtriangles.end());
  auto cdf = sample_triangles_cdf(triangles, shape->positions);
  auto rng = make_rng(19873991);
  for (auto idx = 0; idx < num; idx++) {
    auto [elem, uv] = sample_triangles(cdf, rand1f(rng), rand2f(rng));
    auto q          = triangles[elem];
    positions.push_back(interpolate_triangle(shape->positions[q.x],
        shape->positions[q.y], shape->positions[q.z], uv));
    normals.push_back(normalize(interpolate_triangle(
        shape->normals[q.x], shape->normals[q.y], shape->normals[q.z], uv)));
    if (!texcoords.empty()) {
      texcoords.push_back(interpolate_triangle(shape->texcoords[q.x],
          shape->texcoords[q.y], shape->texcoords[q.z], uv));
    } else {
      texcoords.push_back(uv);
    }
  }
}

struct terrain_params {
  float size    = 0.1f;
  vec3f center  = zero3f;
  float height  = 0.1f;
  float scale   = 10;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{154, 205, 50, 255} / 255);
  vec4f middle  = srgb_to_rgb(vec4f{205, 133, 63, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{240, 255, 255, 255} / 255);
};

void make_terrain(sceneio_scene* scene, sceneio_instance* instance,
    const terrain_params& params) {
  int   size       = instance->shape->positions.size();
  float altezzamax = 0;
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = (1 - length(position - params.center) / params.size) *
             ridge(position * params.scale, params.octaves) * params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    if (position.y > altezzamax) altezzamax = position.y;
  }
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto altezza  = position.y / altezzamax;
    if (altezza <= 0.3)
      instance->shape->colors.push_back(params.bottom);
    else if (altezza > 0.3 && altezza <= 0.6)
      instance->shape->colors.push_back(params.middle);
    else
      instance->shape->colors.push_back(params.top);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

struct displacement_params {
  float height  = 0.02f;
  float scale   = 50;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{64, 224, 208, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_displacement(sceneio_scene* scene, sceneio_instance* instance,
    const displacement_params& params) {
  int size = instance->shape->positions.size();
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = turbulence(position * params.scale, params.octaves) *
             params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    auto color = interpolate_line(params.bottom, params.top, h / params.height);
    instance->shape->colors.push_back(color);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

struct hair_params {
  int   num      = 100000;
  int   steps    = 1;
  float lenght   = 0.02f;
  float scale    = 250;
  float strength = 0.01f;
  float gravity  = 0.0f;
  vec4f bottom   = srgb_to_rgb(vec4f{25, 25, 25, 255} / 255);
  vec4f top      = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_hair(sceneio_scene* scene, sceneio_instance* instance,
    sceneio_instance* hair, const hair_params& params) {
  hair->shape = add_shape(scene, "hair");
  sample_shape(instance->shape->positions, instance->shape->normals,
      instance->shape->texcoords, instance->shape,
      params.num - instance->shape->positions.size());
  int size = instance->shape->positions.size();
  for (auto i = 0; i < size; i++) {
    auto position       = instance->shape->positions[i];
    auto finalPositions = vector<vec3f>{};
    auto colors         = vector<vec4f>{};
    finalPositions.push_back(position);
    colors.push_back(params.bottom);

    for (auto j = 1; j < params.steps + 1; j++) {
      auto finalPos = noise3(position * params.scale) * params.strength +
                      (params.lenght / params.steps) *
                          instance->shape->normals[i] +
                      position;
      finalPos.y -= params.gravity;
      auto color = lerp(params.bottom, params.top, j / ((float)params.steps));
      instance->shape->normals[i] = normalize(finalPos - position);
      finalPositions.push_back(finalPos);
      colors.push_back(color);
      position = finalPos;
    }
    add_polyline(hair->shape, finalPositions, colors);
  }
  instance->shape->normals = compute_tangents(
      instance->shape->lines, instance->shape->positions);
}

struct grass_params {
  int num = 10000;
};

void make_grass(sceneio_scene* scene, sceneio_instance* object,
    const vector<sceneio_instance*>& grasses, const grass_params& params) {
  sample_shape(object->shape->positions, object->shape->normals,
      object->shape->texcoords, object->shape, params.num);
  int  size = object->shape->positions.size();
  auto rng  = make_rng(172842);
  for (auto i = 0; i < size; i++) {
    auto grass = add_instance(scene);

    auto random  = floor(rand1f(rng) * (grasses.size() - 1));
    auto modello = grasses[random];

    grass->frame    = modello->frame;
    grass->shape    = modello->shape;
    grass->material = modello->material;
    auto  position  = object->shape->positions[i];
    float roty      = (float)rand1f(rng) * 2.0f * pi;
    float rotz      = (float)rand1f(rng) / 10.0f + 0.1f;
    auto  scale     = vec3f{rand3f(rng) / 10.0f + 0.9f};
    grass->frame *= translation_frame(position) * scaling_frame(vec3f{scale}) *
                    rotation_frame(vec3f{0, 1, 0}, roty) *
                    rotation_frame(vec3f{0, 0, 1}, rotz);
  }
}

struct voronoise_params {
  float height  = 0.02f;
  float scale   = 50;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{64, 224, 208, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_voronoise(sceneio_scene* scene, sceneio_instance* instance,
    const voronoise_params& params) {
  int size = instance->shape->positions.size();
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = voronoise(position * params.scale, 1, 1) * params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    auto color = interpolate_line(params.bottom, params.top, h / params.height);
    instance->shape->colors.push_back(color);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

struct smoothvoronoi_params {
  float height  = 0.02f;
  float scale   = 50;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{64, 224, 208, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_smoothvoronoi(sceneio_scene* scene, sceneio_instance* instance,
    const smoothvoronoi_params& params) {
  int size = instance->shape->positions.size();
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = smoothvoronoi(position * params.scale) * params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    auto color = interpolate_line(params.bottom, params.top, h / params.height);
    instance->shape->colors.push_back(color);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

struct voronoiedges_params {
  float height  = 0.02f;
  float scale   = 50;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{64, 224, 208, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_voronoiedges(sceneio_scene* scene, sceneio_instance* instance,
    const voronoiedges_params& params) {
  int size = instance->shape->positions.size();
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = getBorder(position * params.scale) * params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    auto color = interpolate_line(params.bottom, params.top, h / params.height);
    instance->shape->colors.push_back(color);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

struct poxo_params {
  float height  = 0.02f;
  float scale   = 50;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{64, 224, 208, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_poxo(sceneio_scene* scene, sceneio_instance* instance,
    const poxo_params& params) {
  int size = instance->shape->positions.size();
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = poxo(position * params.scale) * params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    auto color = interpolate_line(params.bottom, params.top, h / params.height);
    instance->shape->colors.push_back(color);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

struct cellnoise_params {
  float height  = 0.02f;
  float scale   = 50;
  int   octaves = 8;
  vec4f bottom  = srgb_to_rgb(vec4f{64, 224, 208, 255} / 255);
  vec4f top     = srgb_to_rgb(vec4f{244, 164, 96, 255} / 255);
};

void make_cellnoise(sceneio_scene* scene, sceneio_instance* instance,
    const cellnoise_params& params) {
  int size = instance->shape->positions.size();
  for (int i = 0; i < size; i++) {
    auto position = instance->shape->positions[i];
    auto h        = cellnoise(position * params.scale) * params.height;
    position += instance->shape->normals[i] * h;
    instance->shape->positions[i] = position;
    auto color = interpolate_line(params.bottom, params.top, h / params.height);
    instance->shape->colors.push_back(color);
  }
  instance->shape->normals = compute_normals(
      instance->shape->quads, instance->shape->positions);
}

int main(int argc, const char* argv[]) {
  // command line parameters
  auto terrain        = ""s;
  auto tparams        = terrain_params{};
  auto displacement   = ""s;
  auto dparams        = displacement_params{};
  auto hair           = ""s;
  auto hairbase       = ""s;
  auto hparams        = hair_params{};
  auto grass          = ""s;
  auto grassbase      = ""s;
  auto gparams        = grass_params{};
  auto voronoise      = ""s;
  auto vparams        = voronoise_params{};
  auto poxo           = ""s;
  auto pparams        = poxo_params{};
  auto smoothvoronoi  = ""s;
  auto smoothvparams  = smoothvoronoi_params{};
  auto voronoiedges   = ""s;
  auto voredgesparams = voronoiedges_params{};
  auto cellnoise      = ""s;
  auto cellparams     = cellnoise_params{};
  auto output         = "out.json"s;
  auto filename       = "scene.json"s;

  // parse command line
  auto cli = make_cli("yscenegen", "Make procedural scenes");
  add_option(cli, "--terrain", terrain, "terrain object");
  add_option(cli, "--displacement", displacement, "displacement object");
  add_option(cli, "--hair", hair, "hair object");
  add_option(cli, "--hairbase", hairbase, "hairbase object");
  add_option(cli, "--grass", grass, "grass object");
  add_option(cli, "--grassbase", grassbase, "grassbase object");
  add_option(cli, "--voronoise", voronoise, "voronoise object");
  add_option(cli, "--poxo", poxo, "poxo object");
  add_option(cli, "--smoothvoronoi", smoothvoronoi, "smoothvoronoi object");
  add_option(cli, "--voronoiedges", voronoiedges, "voronoiedges object");
  add_option(cli, "--cellnoise", cellnoise, "cellnoise object");
  add_option(cli, "--hairnum", hparams.num, "hair number");
  add_option(cli, "--hairlen", hparams.lenght, "hair length");
  add_option(cli, "--hairstr", hparams.strength, "hair strength");
  add_option(cli, "--hairgrav", hparams.gravity, "hair gravity");
  add_option(cli, "--hairstep", hparams.steps, "hair steps");
  add_option(cli, "--output,-o", output, "output scene");
  add_option(cli, "scene", filename, "input scene", true);
  parse_cli(cli, argc, argv);

  // load scene
  auto scene_guard = std::make_unique<sceneio_scene>();
  auto scene       = scene_guard.get();
  auto ioerror     = ""s;
  if (!load_scene(filename, scene, ioerror, print_progress))
    print_fatal(ioerror);

  // create procedural geometry
  if (terrain != "") {
    make_terrain(scene, get_instance(scene, terrain), tparams);
  }
  if (displacement != "") {
    make_displacement(scene, get_instance(scene, displacement), dparams);
  }
  if (hair != "") {
    make_hair(scene, get_instance(scene, hairbase), get_instance(scene, hair),
        hparams);
  }
  if (grass != "") {
    auto grasses = vector<sceneio_instance*>{};
    for (auto instance : scene->instances)
      if (instance->name.find(grass) != scene->name.npos)
        grasses.push_back(instance);
    make_grass(scene, get_instance(scene, grassbase), grasses, gparams);
  }
  if (voronoise != "") {
    make_voronoise(scene, get_instance(scene, voronoise), vparams);
  }
  if (smoothvoronoi != "") {
    make_smoothvoronoi(
        scene, get_instance(scene, smoothvoronoi), smoothvparams);
  }
  if (voronoiedges != "") {
    make_voronoiedges(scene, get_instance(scene, voronoiedges), voredgesparams);
  }
  if (poxo != "") {
    make_poxo(scene, get_instance(scene, poxo), pparams);
  }
  if (cellnoise != "") {
    make_cellnoise(scene, get_instance(scene, cellnoise), cellparams);
  }

  // make a directory if needed
  if (!make_directory(path_dirname(output), ioerror)) print_fatal(ioerror);
  if (!scene->shapes.empty()) {
    if (!make_directory(path_join(path_dirname(output), "shapes"), ioerror))
      print_fatal(ioerror);
  }
  if (!scene->textures.empty()) {
    if (!make_directory(path_join(path_dirname(output), "textures"), ioerror))
      print_fatal(ioerror);
  }

  // save scene
  if (!save_scene(output, scene, ioerror, print_progress)) print_fatal(ioerror);

  // done
  return 0;
}
