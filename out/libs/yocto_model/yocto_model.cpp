//
// Implementation for Yocto/Model
//

//
// LICENSE:
//
// Copyright (c) 2016 -- 2021 Fabio Pellacini
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

// -----------------------------------------------------------------------------
// INCLUDES
// -----------------------------------------------------------------------------

#include "yocto_model.h"

#include <yocto/yocto_sampling.h>

#include <iostream>

#include "ext/perlin-noise/noise1234.h"

// -----------------------------------------------------------------------------
// USING DIRECTIVES
// -----------------------------------------------------------------------------
namespace yocto {

// using directives
using std::array;
using std::string;
using std::vector;
using namespace std::string_literals;

}  // namespace yocto

// -----------------------------------------------------------------------------
// IMPLEMENTATION FOR EXAMPLE OF PROCEDURAL MODELING
// -----------------------------------------------------------------------------
namespace yocto {

float noise(const vec3f& p) { return ::noise3(p.x, p.y, p.z); }
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

void add_polyline(shape_data& shape, const vector<vec3f>& positions,
    const vector<vec4f>& colors, float thickness = 0.0001f) {
  auto offset = (int)shape.positions.size();
  shape.positions.insert(
      shape.positions.end(), positions.begin(), positions.end());
  shape.colors.insert(shape.colors.end(), colors.begin(), colors.end());
  shape.radius.insert(shape.radius.end(), positions.size(), thickness);
  for (auto idx = 0; idx < positions.size() - 1; idx++) {
    shape.lines.push_back({offset + idx, offset + idx + 1});
  }
}

void sample_shape(vector<vec3f>& positions, vector<vec3f>& normals,
    vector<vec2f>& texcoords, const shape_data& shape, int num) {
  auto triangles  = shape.triangles;
  auto qtriangles = quads_to_triangles(shape.quads);
  triangles.insert(triangles.end(), qtriangles.begin(), qtriangles.end());
  auto cdf = sample_triangles_cdf(triangles, shape.positions);
  auto rng = make_rng(19873991);
  for (auto idx = 0; idx < num; idx++) {
    auto [elem, uv] = sample_triangles(cdf, rand1f(rng), rand2f(rng));
    auto q          = triangles[elem];
    positions.push_back(interpolate_triangle(
        shape.positions[q.x], shape.positions[q.y], shape.positions[q.z], uv));
    normals.push_back(normalize(interpolate_triangle(
        shape.normals[q.x], shape.normals[q.y], shape.normals[q.z], uv)));
    if (!texcoords.empty()) {
      texcoords.push_back(interpolate_triangle(shape.texcoords[q.x],
          shape.texcoords[q.y], shape.texcoords[q.z], uv));
    } else {
      texcoords.push_back(uv);
    }
  }
}

struct density_mapped_shape_data {
  shape_data    shape;
  vector<float> density_map;
};

vector<float> sample_triangles_density_cdf(const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<float>& density_map) {
  auto cdf = vector<float>(triangles.size());
  for (auto i = 0; i < cdf.size(); i++) {
    auto& t = triangles[i];
    auto  w = triangle_area(positions[t.x], positions[t.y], positions[t.z]);

    cdf[i] = w * (abs(density_map[t.x] + density_map[t.z] + density_map[t.y])) +
             (i != 0 ? cdf[i - 1] : 0);
  }
  auto sum = 0.0f;
  for (auto& el : cdf) {
    sum += el;
    std::cout << el << std::endl;
  }
  return cdf;
}

void sample_shape_mapped(vector<vec3f>& positions, vector<vec3f>& normals,
    vector<vec2f>& texcoords, const density_mapped_shape_data& shape, int num) {
  auto triangles  = shape.shape.triangles;
  auto qtriangles = quads_to_triangles(shape.shape.quads);

  triangles.insert(triangles.end(), qtriangles.begin(), qtriangles.end());

  auto cdf = sample_triangles_density_cdf(
      triangles, shape.shape.positions, shape.density_map);
  auto rng = make_rng((int)(0xC + 0x1 + 0xA + 0));
  for (auto idx = 0; idx < num; idx++) {
    auto [elem, uv] = sample_triangles(cdf, rand1f(rng), rand2f(rng));
    auto q          = triangles[elem];
    positions.push_back(interpolate_triangle(shape.shape.positions[q.x],
        shape.shape.positions[q.y], shape.shape.positions[q.z], uv));
    normals.push_back(normalize(interpolate_triangle(shape.shape.normals[q.x],
        shape.shape.normals[q.y], shape.shape.normals[q.z], uv)));
    if (!texcoords.empty()) {
      texcoords.push_back(interpolate_triangle(shape.shape.texcoords[q.x],
          shape.shape.texcoords[q.y], shape.shape.texcoords[q.z], uv));
    } else {
      texcoords.push_back(uv);
    }
  }
}

void make_dense_hair(scene_data& scene, shape_data& hair,
    const instance_data& object, const hair_params& params) {
  auto          material       = scene.materials[object.material];
  auto          shape          = scene.shapes[object.shape];
  auto          segment_length = params.lenght / params.steps;
  vector<vec3f> positions;
  vector<vec3f> normals;
  vector<vec2f> texcoords;
  vector<float> density_map;
  for (int i = 0; i < shape.positions.size(); i++) {
    auto texture_value = eval_texture(scene, material.color_tex, texcoords[i]);
    density_map.push_back(
        (texture_value.x + texture_value.y + texture_value.z) / 3);
  }
  sample_shape_mapped(
      positions, normals, texcoords, {shape, density_map}, params.num);
  for (int i = 0; i < params.num; i++) {
    vector<vec3f> point_list;
    vector<vec4f> colors;
    vec3f         old_point;  // punto iniziale
    vec3f         next_point = positions[i];
    auto          norm       = normals[i];  // normale iniziale

    for (int s = 0; s <= params.steps; s++) {
      // pusho l'attuale punto
      old_point = next_point;
      point_list.push_back(next_point);
      auto color_mult = s * segment_length / params.lenght;
      colors.push_back(
          (1 - color_mult) * params.bottom + color_mult * params.top);
      // calcolo i dati per il prssimo punto
      next_point = ray_point(ray3f{old_point, norm}, segment_length);
      next_point += noise3(old_point * params.scale) * params.strength;
      next_point.y -= params.gravity;
      norm = normalize(next_point - old_point);
    }
    add_polyline(hair, point_list, colors);
  }
  hair.normals = compute_normals(hair);
}

void make_terrain(shape_data& shape, const terrain_params& params) {
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = ridge(pos * params.scale, params.octaves) * params.height *
                (1 - length(pos - params.center) / params.size);
    pos += norm * molt;

    // color
    auto height = molt / params.height;
    auto color  = params.top;
    if (height < 0.3)
      color = params.bottom;
    else if (height < 0.6)
      color = params.middle;
    shape.colors.push_back(color);
  }
  // normals
  shape.normals = compute_normals(shape);
}

void make_displacement(shape_data& shape, const displacement_params& params) {
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = turbulence(pos * params.scale, params.octaves) * params.height;
    pos += norm * molt;

    // color
    auto height = molt / params.height;
    auto color  = height * params.top + (1 - height) * params.bottom;
    shape.colors.push_back(color);
  }
  // normals
  shape.normals = compute_normals(shape);
}

void make_hair(
    shape_data& hair, const shape_data& shape, const hair_params& params) {
  auto          segment_length = params.lenght / params.steps;
  vector<vec3f> positions;
  vector<vec3f> normals;
  vector<vec2f> texcoords;
  sample_shape(positions, normals, texcoords, shape, params.num);
  for (int i = 0; i < params.num; i++) {
    vector<vec3f> point_list;
    vector<vec4f> colors;
    vec3f         old_point;  // punto iniziale
    vec3f         next_point = positions[i];
    auto          norm       = normals[i];  // normale iniziale

    for (int s = 0; s <= params.steps; s++) {
      // pusho l'attuale punto
      old_point = next_point;
      point_list.push_back(next_point);
      auto color_mult = s * segment_length / params.lenght;
      colors.push_back(
          (1 - color_mult) * params.bottom + color_mult * params.top);
      // calcolo i dati per il prssimo punto
      next_point = ray_point(ray3f{old_point, norm}, segment_length);
      next_point += noise3(old_point * params.scale) * params.strength;
      next_point.y -= params.gravity;
      norm = normalize(next_point - old_point);
    }
    add_polyline(hair, point_list, colors);
  }
  hair.normals = compute_normals(hair);
}

void make_grass(scene_data& scene, const instance_data& object,
    const vector<instance_data>& grasses, const grass_params& params) {
  instance_data hair;
  hair_params   p;
  make_dense_hair(scene, scene.shapes[hair.shape], object, p);
  return;
  vector<vec3f> positions;
  vector<vec3f> normals;
  vector<vec2f> texcoords;
  auto          rng = rng_state(69420, 666);
  sample_shape(
      positions, normals, texcoords, scene.shapes[object.shape], params.num);
  for (int i = 0; i < params.num; i++) {
    int           index     = (int)(rand1i(rng, grasses.size()));
    instance_data new_grass = grasses[index];
    new_grass.frame.y       = normals[i];
    new_grass.frame.x       = normalize(
              vec3f{1, 0, 0} -
              dot(vec3f{1, 0, 0}, new_grass.frame.y) * new_grass.frame.y);
    new_grass.frame.z = cross(new_grass.frame.x, new_grass.frame.y);
    new_grass.frame.o = positions[i];

    float scale_factor = 0.9f + rand1f(rng) * 0.1;
    new_grass.frame *= scaling_frame(
        vec3f{scale_factor, scale_factor, scale_factor});

    float rotate_y = rand1f(rng) * 2 * pif;
    new_grass.frame *= rotation_frame(new_grass.frame.y, rotate_y);

    float rotate_z = 0.1f + rand1f(rng) * 0.1f;
    new_grass.frame *= rotation_frame(new_grass.frame.z, rotate_z);

    scene.instances.push_back(new_grass);
  }
}

}  // namespace yocto
