//
// Implementation for Yocto/Particle.
//

//
// LICENSE:
//
// Copyright (c) 2020 -- 2020 Fabio Pellacini
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//

#include "yocto_particle.h"

#include <yocto/yocto_geometry.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_shape.h>

#include <stdexcept>
#include <unordered_set>
// -----------------------------------------------------------------------------
// SIMULATION DATA AND API
// -----------------------------------------------------------------------------
namespace yocto {

// cleanup
particle_scene::~particle_scene() {
  for (auto shape : shapes) delete shape;
  for (auto collider : colliders) delete collider;
}

// Scene creation
particle_shape* add_shape(particle_scene* scene) {
  return scene->shapes.emplace_back(new particle_shape{});
}
particle_collider* add_collider(particle_scene* scene) {
  return scene->colliders.emplace_back(new particle_collider{});
}
particle_shape* add_particles(particle_scene* scene, const vector<int>& points,
    const vector<vec3f>& positions, const vector<float>& radius, float mass,
    float random_velocity) {
  auto shape               = add_shape(scene);
  shape->points            = points;
  shape->initial_positions = positions;
  shape->initial_normals.assign(shape->positions.size(), {0, 0, 1});
  shape->initial_radius = radius;
  shape->initial_invmass.assign(
      positions.size(), 1 / (mass * positions.size()));
  shape->initial_velocities.assign(positions.size(), {0, 0, 0});
  shape->emit_rngscale = random_velocity;
  // avoid crashes
  shape->positions = shape->initial_positions;
  shape->normals   = shape->normals;
  shape->radius    = shape->initial_radius;
  return shape;
}
particle_shape* add_cloth(particle_scene* scene, const vector<vec4i>& quads,
    const vector<vec3f>& positions, const vector<vec3f>& normals,
    const vector<float>& radius, float mass, float coeff,
    const vector<int>& pinned) {
  auto shape               = add_shape(scene);
  shape->quads             = quads;
  shape->initial_positions = positions;
  shape->initial_normals   = normals;
  shape->initial_radius    = radius;
  shape->initial_invmass.assign(
      positions.size(), 1 / (mass * positions.size()));
  shape->initial_velocities.assign(positions.size(), {0, 0, 0});
  shape->initial_pinned = pinned;
  shape->spring_coeff   = coeff;
  // avoid crashes
  shape->positions = shape->initial_positions;
  shape->normals   = shape->normals;
  shape->radius    = shape->initial_radius;
  return shape;
}
particle_collider* add_collider(particle_scene* scene,
    const vector<vec3i>& triangles, const vector<vec4i>& quads,
    const vector<vec3f>& positions, const vector<vec3f>& normals,
    const vector<float>& radius) {
  auto collider       = add_collider(scene);
  collider->quads     = quads;
  collider->triangles = triangles;
  collider->positions = positions;
  collider->normals   = normals;
  collider->radius    = radius;
  return collider;
}

// Set shapes
void set_velocities(
    particle_shape* shape, const vec3f& velocity, float random_scale) {
  shape->emit_velocity = velocity;
  shape->emit_rngscale = random_scale;
}

// Get shape properties
void get_positions(particle_shape* shape, vector<vec3f>& positions) {
  positions = shape->positions;
}
void get_normals(particle_shape* shape, vector<vec3f>& normals) {
  normals = shape->normals;
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// SIMULATION DATA AND API
// -----------------------------------------------------------------------------
namespace yocto {

// Init simulation
void init_simulation(particle_scene* scene, const particle_params& params) {
  // COPY INITIAL VALUES
  auto rng = make_rng(params.seed);
  for (auto& shape : scene->shapes) {
    shape->positions  = shape->initial_positions;
    shape->normals    = shape->initial_normals;
    shape->radius     = shape->initial_radius;
    shape->velocities = shape->initial_velocities;
    shape->invmass    = shape->initial_invmass;
    shape->forces.resize(shape->invmass.size());

    // SETUP PINNED

    for (auto v : shape->initial_pinned) {
      shape->invmass[v] = 0;
    }

    // INITIALIZE VELOCITIES

    for (auto v = 0; v < shape->velocities.size(); v++) {
      shape->velocities[v] += rand1f(rng) * shape->emit_rngscale *
                              sample_sphere(rand2f(rng));
    }

    // MAKE SPRINGS
    if (shape->spring_coeff > 0) {
      if (!shape->quads.empty()) {
        for (auto edge : get_edges(shape->quads)) {
          shape->springs.push_back({edge.x, edge.y,
              distance(shape->positions[edge.x], shape->positions[edge.y]),
              shape->spring_coeff});
        }

        for (auto quad : shape->quads) {
          shape->springs.push_back({quad.x, quad.z,
              distance(shape->positions[quad.x], shape->positions[quad.z]),
              shape->spring_coeff});
          shape->springs.push_back({quad.y, quad.w,
              distance(shape->positions[quad.y], shape->positions[quad.w]),
              shape->spring_coeff});
        }
      } else if (!shape->triangles.empty()) {
        for (auto edge : get_edges(shape->triangles)) {
          shape->springs.push_back({edge.x, edge.y,
              distance(shape->positions[edge.x], shape->positions[edge.y]),
              shape->spring_coeff});
        }
      }
    }
  }
  // INITIALIZE COLLIDERS BVH

  for (auto& collider : scene->colliders) {
    if (!collider->quads.empty()) {
      collider->bvh = make_quads_bvh(
          collider->quads, collider->positions, collider->radius);
    } else if (!collider->triangles.empty()) {
      collider->bvh = make_triangles_bvh(
          collider->triangles, collider->positions, collider->radius);
    }
  }
}

// check if a point is inside a collider
bool collide_collider(particle_collider* collider, const vec3f& position,
    vec3f& hit_position, vec3f& hit_normal) {
  auto ray = ray3f{position, vec3f{0, 1, 0}};

  if (!collider->quads.empty()) {
    auto isec = intersect_quads_bvh(
        collider->bvh, collider->quads, collider->positions, ray);
    if (!isec.hit) return false;
    auto q       = collider->quads[isec.element];
    hit_position = interpolate_quad(collider->positions[q.x],
        collider->positions[q.y], collider->positions[q.z],
        collider->positions[q.w], isec.uv);
    hit_normal   = normalize(
        interpolate_quad(collider->normals[q.x], collider->normals[q.y],
            collider->normals[q.z], collider->normals[q.w], isec.uv));

  } else if (!collider->triangles.empty()) {
    auto isec = intersect_triangles_bvh(
        collider->bvh, collider->triangles, collider->positions, ray);
    if (!isec.hit) return false;
    auto t       = collider->triangles[isec.element];
    hit_position = interpolate_triangle(collider->positions[t.x],
        collider->positions[t.y], collider->positions[t.z], isec.uv);
    hit_normal   = normalize(interpolate_triangle(collider->normals[t.x],
        collider->normals[t.y], collider->normals[t.z], isec.uv));
  }

  return dot(hit_normal, ray.d) > 0;
}

// simulate mass-spring
void simulate_massspring(particle_scene* scene, const particle_params& params) {
  // SAVE OLD POSITIONS
  for (auto& particle : scene->shapes) {
    particle->old_positions = particle->positions;

    // COMPUTE DYNAMICS
    for (int s = 0; s < params.mssteps; s++) {
      auto ddt = params.deltat / params.mssteps;
      //<compute forces>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;
        vec3f g             = {0, -params.gravity, 0};
        particle->forces[i] = g / particle->invmass[i];
      }

      for (auto& spring : particle->springs) {
        auto& particle0 = spring.vert0;
        auto& particle1 = spring.vert1;
        auto  invmass   = particle->invmass[particle0] +
                       particle->invmass[particle1];

        if (!invmass) continue;

        auto delta_pos = particle->positions[particle1] -
                         particle->positions[particle0];
        auto spring_dir = normalize(delta_pos);
        auto spring_len = length(delta_pos);
        auto force      = spring_dir * (spring_len / spring.rest - 1) /
                     (spring.coeff * invmass);

        auto delta_vel = particle->velocities[particle1] -
                         particle->velocities[particle0];
        force += dot(delta_vel / spring.rest, spring_dir) * spring_dir /
                 (spring.coeff * 1000 * invmass);

        particle->forces[particle0] += force;
        particle->forces[particle1] -= force;
      }
      //<update state using semi-implicit Euler’s>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->velocities[i] += ddt * particle->forces[i] *
                                   particle->invmass[i];
        particle->positions[i] += ddt * particle->velocities[i];
      }
    }

    // HANDLE COLLISIONS
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      for (auto collider : scene->colliders) {
        auto hitpos = zero3f, hit_normal = zero3f;

        if (collide_collider(
                collider, particle->positions[i], hitpos, hit_normal)) {
          particle->positions[i] = hitpos + hit_normal * 0.005;
          auto projection        = dot(particle->velocities[i], hit_normal);

          particle->velocities[i] =
              (particle->velocities[i] - projection * hit_normal) *
                  (1 - params.bounce.x) -
              projection * hit_normal * (1 - params.bounce.y);
        }
      }
    }

    // VELOCITY FILTER

    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // damping

      particle->velocities[i] *= (1 - params.dumping * params.deltat);
      // sleeping
      if (length(particle->velocities[i]) < params.minvelocity)
        particle->velocities[i] = {0, 0, 0};
    }

    // RECOMPUTE NORMALS
    if (!particle->quads.empty())
      particle->normals = compute_normals(particle->quads, particle->positions);
    else if (!particle->triangles.empty()) {
      particle->normals = compute_normals(
          particle->triangles, particle->positions);
    }
  }
}
// simulate pbd
void simulate_pbd(particle_scene* scene, const particle_params& params) {
  // SAVE OLD POSITOINS
  for (auto& particle : scene->shapes) {
    particle->old_positions = particle->positions;

    // PREDICT POSITIONS
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // apply semi-implicit Euler to external forces
      particle->velocities[i] += vec3f{0, -params.gravity, 0} * params.deltat;
      particle->positions[i] += particle->velocities[i] * params.deltat;
    }
    // COMPUTE COLLISIONS
    particle->collisions.clear();
    for (auto i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      for (auto collider : scene->colliders) {
        auto hit_position = zero3f, hit_normal = zero3f;
        if (!collide_collider(
                collider, particle->positions[i], hit_position, hit_normal)) {
          continue;
        }

        particle->collisions.push_back({i, hit_position, hit_normal});
      }
    }
    // SOLVE CONSTRAINTS
    for (auto i = 0; i < params.pdbsteps; i++) {
      //<solve springs>
      for (auto& spring : particle->springs) {
        auto& particle0 = spring.vert0;
        auto& particle1 = spring.vert1;
        auto  invmass   = particle->invmass[particle1] +
                       particle->invmass[particle0];
        if (!invmass) continue;
        auto dir = particle->positions[particle1] -
                   particle->positions[particle0];
        auto len = length(dir);
        dir /= len;
        auto lambda = (1 - spring.coeff) * (len - spring.rest) / invmass;
        particle->positions[particle0] += particle->invmass[particle0] *
                                          lambda * dir;
        particle->positions[particle1] -= particle->invmass[particle1] *
                                          lambda * dir;
      }
      //<solve collisions>
      for (auto& collision : particle->collisions) {
        auto& particle0 = collision.vert;
        if (!particle->invmass[particle0]) continue;
        auto projection = dot(
            particle->positions[particle0] - collision.position,
            collision.normal);
        if (projection >= 0) continue;
        particle->positions[particle0] += -projection * collision.normal;
      }
    }
    // COMPUTE VELOCITIES
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      particle->velocities[i] =
          (particle->positions[i] - particle->old_positions[i]) / params.deltat;
    }
    // VELOCITY FILTER
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // damping
      particle->velocities[i] *= (1 - params.dumping * params.deltat);
      // sleeping
      if (length(particle->velocities[i]) < params.minvelocity) {
        particle->velocities[i] = zero3f;
      }
    }
    // RECOMPUTE NORMALS
    if (!particle->quads.empty()) {
      particle->normals = compute_normals(particle->quads, particle->positions);
    } else if (!particle->triangles.empty()) {
      particle->normals = compute_normals(
          particle->triangles, particle->positions);
    }
  }
}

void simulate_myeffect(particle_scene* scene, const particle_params& params) {
  // SAVE OLD POSITIONS
  for (auto& particle : scene->shapes) {
    particle->old_positions = particle->positions;

    // COMPUTE DYNAMICS
    for (int s = 0; s < params.mssteps; s++) {
      auto ddt = params.deltat / params.mssteps;
      //<compute forces>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->positions[i].x = particle->positions[i].x * cosf(0.0001) -
                                   particle->positions[i].z * sinf(0.0001);

        particle->positions[i].z = particle->positions[i].x * sinf(0.0001) +
                                   particle->positions[i].z * cosf(0.0001);
        particle->positions[i].y =
            max(abs(particle->positions[i].x), abs(particle->positions[i].z)) *
            2;
      }

      //<update state using semi-implicit Euler’s>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->velocities[i] += ddt * particle->forces[i] *
                                   particle->invmass[i];

        particle->positions[i] += ddt * particle->velocities[i];
      }
    }

    // HANDLE COLLISIONS
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      for (auto collider : scene->colliders) {
        auto hitpos = zero3f, hit_normal = zero3f;

        if (collide_collider(
                collider, particle->positions[i], hitpos, hit_normal)) {
          particle->positions[i] = hitpos;
          auto projection        = dot(particle->velocities[i], hit_normal);

          particle->velocities[i] =
              (particle->velocities[i] - projection * hit_normal) *
                  (1 - params.bounce.x) -
              projection * hit_normal * (1 - params.bounce.y);
        }
      }
    }

    // VELOCITY FILTER

    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // damping

      particle->velocities[i] *= (1 - params.dumping * params.deltat);
      // sleeping
      if (length(particle->velocities[i]) < params.minvelocity)
        particle->velocities[i] = {0, 0, 0};
    }

    // RECOMPUTE NORMALS
    if (!particle->quads.empty())
      particle->normals = compute_normals(particle->quads, particle->positions);
    else if (!particle->triangles.empty()) {
      particle->normals = compute_normals(
          particle->triangles, particle->positions);
    }
  }
}
void simulate_myeffect2(particle_scene* scene, const particle_params& params) {
  for (auto& particle : scene->shapes) {
    particle->old_positions = particle->positions;

    // COMPUTE DYNAMICS
    for (int s = 0; s < params.mssteps; s++) {
      auto ddt = params.deltat / params.mssteps;
      //<compute forces>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        vec3f g             = {-params.gravity * 3, -params.gravity, 0};
        particle->forces[i] = g / particle->invmass[i];
      }

      //<update state using semi-implicit Euler’s>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->velocities[i] += ddt * particle->forces[i] *
                                   particle->invmass[i];

        particle->positions[i] += ddt * particle->velocities[i];
      }
    }

    // HANDLE COLLISIONS
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      for (auto collider : scene->colliders) {
        auto hitpos = zero3f, hit_normal = zero3f;

        if (collide_collider(
                collider, particle->positions[i], hitpos, hit_normal)) {
          particle->positions[i] = hitpos + hit_normal * 0.005;
          auto projection        = dot(particle->velocities[i], hit_normal);

          particle->velocities[i] =
              (particle->velocities[i] - projection * hit_normal) *
                  (1 - params.bounce.x) -
              projection * hit_normal * (1 - params.bounce.y);
        }
      }
    }

    // VELOCITY FILTER

    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // damping

      particle->velocities[i] *= (1 - params.dumping * params.deltat);
      // sleeping
      if (length(particle->velocities[i]) < params.minvelocity)
        particle->velocities[i] = {0, 0, 0};
    }

    // RECOMPUTE NORMALS
    if (!particle->quads.empty())
      particle->normals = compute_normals(particle->quads, particle->positions);
    else if (!particle->triangles.empty()) {
      particle->normals = compute_normals(
          particle->triangles, particle->positions);
    }
  }
}
void simulate_myeffect3(particle_scene* scene, const particle_params& params) {
  // SAVE OLD POSITIONS
  for (auto& particle : scene->shapes) {
    particle->old_positions = particle->positions;

    // COMPUTE DYNAMICS
    for (int s = 0; s < params.mssteps; s++) {
      auto ddt = params.deltat / params.mssteps;
      //<compute forces>
      for (int i = 0; i < particle->invmass.size(); i++) {
        particle->positions[i].y = 0;
      }
      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->positions[i].x = particle->positions[i].x * cosf(0.0001) -
                                   particle->positions[i].z * sinf(0.0001);

        particle->positions[i].z = particle->positions[i].x * sinf(0.0001) +
                                   particle->positions[i].z * cosf(0.0001);
      }

      //<update state using semi-implicit Euler’s>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->velocities[i] += ddt * particle->forces[i] *
                                   particle->invmass[i];

        particle->positions[i] += ddt * particle->velocities[i];
      }
    }

    // HANDLE COLLISIONS
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      for (auto collider : scene->colliders) {
        auto hitpos = zero3f, hit_normal = zero3f;

        if (collide_collider(
                collider, particle->positions[i], hitpos, hit_normal)) {
          particle->positions[i] = hitpos;
          auto projection        = dot(particle->velocities[i], hit_normal);

          particle->velocities[i] =
              (particle->velocities[i] - projection * hit_normal) *
                  (1 - params.bounce.x) -
              projection * hit_normal * (1 - params.bounce.y);
        }
      }
    }

    // VELOCITY FILTER

    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // damping

      particle->velocities[i] *= (1 - params.dumping * params.deltat);
      // sleeping
      if (length(particle->velocities[i]) < params.minvelocity)
        particle->velocities[i] = {0, 0, 0};
    }

    // RECOMPUTE NORMALS
    if (!particle->quads.empty())
      particle->normals = compute_normals(particle->quads, particle->positions);
    else if (!particle->triangles.empty()) {
      particle->normals = compute_normals(
          particle->triangles, particle->positions);
    }
  }
}
void simulate_myeffect4(particle_scene* scene, const particle_params& params) {
  for (auto& particle : scene->shapes) {
    particle->old_positions = particle->positions;

    // COMPUTE DYNAMICS
    for (int s = 0; s < params.mssteps; s++) {
      auto ddt = params.deltat / params.mssteps;
      //<compute forces>
      for (int i = 0; i < particle->invmass.size(); i++) {
        particle->positions[i].y = 0;
      }
      for (int i = 0; i < particle->invmass.size(); i++) {
        vec3f g             = vec3f{0, -params.gravity, 0};
        particle->forces[i] = g / particle->invmass[i];
      }
      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;
        if (particle->positions[i].x >= 0 && particle->positions[i].z >= 0) {
          vec3f g             = vec3f{-1.8, -params.gravity, -1.8};
          particle->forces[i] = g / particle->invmass[i];
          ;
        } else if (particle->positions[i].x < 0 &&
                   particle->positions[i].z >= 0) {
          vec3f g             = vec3f{1.8, -params.gravity, -1.8};
          particle->forces[i] = g / particle->invmass[i];
        } else if (particle->positions[i].x >= 0 &&
                   particle->positions[i].z < 0) {
          vec3f g             = vec3f{-1.8, -params.gravity, 1.8};
          particle->forces[i] = g / particle->invmass[i];
        } else if (particle->positions[i].x < 0 &&
                   particle->positions[i].z < 0) {
          vec3f g             = vec3f{1.8, -params.gravity, 1.8};
          particle->forces[i] = g / particle->invmass[i];
        }
      }
      //<update state using semi-implicit Euler’s>

      for (int i = 0; i < particle->invmass.size(); i++) {
        if (!particle->invmass[i]) continue;

        particle->velocities[i] += ddt * particle->forces[i] *
                                   particle->invmass[i];

        particle->positions[i] += ddt * particle->velocities[i];
      }
    }

    // HANDLE COLLISIONS
    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      for (auto collider : scene->colliders) {
        auto hitpos = zero3f, hit_normal = zero3f;

        if (collide_collider(
                collider, particle->positions[i], hitpos, hit_normal)) {
          particle->positions[i] = hitpos;
          auto projection        = dot(particle->velocities[i], hit_normal);

          particle->velocities[i] =
              (particle->velocities[i] - projection * hit_normal) *
                  (1 - params.bounce.x) -
              projection * hit_normal * (1 - params.bounce.y);
        }
      }
    }

    // VELOCITY FILTER

    for (int i = 0; i < particle->invmass.size(); i++) {
      if (!particle->invmass[i]) continue;
      // damping

      particle->velocities[i] *= (1 - params.dumping * params.deltat);
      // sleeping
      if (length(particle->velocities[i]) < params.minvelocity)
        particle->velocities[i] = {0, 0, 0};
    }

    // RECOMPUTE NORMALS
    if (!particle->quads.empty())
      particle->normals = compute_normals(particle->quads, particle->positions);
    else if (!particle->triangles.empty()) {
      particle->normals = compute_normals(
          particle->triangles, particle->positions);
    }
  }
}
// Simulate one step
void simulate_frame(particle_scene* scene, const particle_params& params) {
  switch (params.solver) {
    case particle_solver_type::mass_spring:
      return simulate_massspring(scene, params);
    case particle_solver_type::position_based:
      return simulate_pbd(scene, params);
    case particle_solver_type::my_effect:
      return simulate_myeffect(scene, params);
    case particle_solver_type::my_effect2:
      return simulate_myeffect2(scene, params);
    case particle_solver_type::my_effect3:
      return simulate_myeffect3(scene, params);
    case particle_solver_type::my_effect4:
      return simulate_myeffect4(scene, params);
    default: throw std::invalid_argument("unknown solver");
  }
}

// Simulate the whole sequence
void simulate_frames(particle_scene* scene, const particle_params& params,
    progress_callback progress_cb) {
  // handle progress
  auto progress = vec2i{0, 1 + (int)params.frames};

  if (progress_cb) progress_cb("init simulation", progress.x++, progress.y);
  init_simulation(scene, params);

  for (auto idx = 0; idx < params.frames; idx++) {
    if (progress_cb) progress_cb("simulate frames", progress.x++, progress.y);
    simulate_frame(scene, params);
  }

  if (progress_cb) progress_cb("simulate frames", progress.x++, progress.y);
}

}  // namespace yocto
