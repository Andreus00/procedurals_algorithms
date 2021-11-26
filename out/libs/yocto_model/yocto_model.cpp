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

///////////////////////////// voronoise

vec4f sin(vec4f x) { return vec4f{sinf(x.x), sinf(x.y), sinf(x.z), sinf(x.w)}; }
// float floor(float x) { return (int)x; }
vec4f floor4(vec4f x) {
  return vec4f{(float)floor((double)x.x), (float)floor((double)x.y),
      (float)floor((double)x.z), (float)floor((double)x.w)};
}
vec3f floor3(vec3f x) {
  return vec3f{(float)floor((double)x.x), (float)floor((double)x.y),
      (float)floor((double)x.z)};
}
vec4f fract4(vec4f x) { return x - floor4(x); }
vec3f fract3(vec3f x) { return x - floor3(x); }

vec4f hash4(vec3f p) {
  vec4f q = vec4f{dot(p, vec3f{127.1, 311.7, 411.3}),
      dot(p, vec3f{269.5, 183.3, 152.4}), dot(p, vec3f{419.2, 371.9, 441.0}),
      dot(p, vec3f{512.4, 321.9, 69.420})};
  return fract4(sin(q) * 43758.5453);
}

float voronoise(vec3f x, float u, float v) {
  vec3f floor_point = vec3f{floor(x.x), floor(x.y), floor(x.z)};
  vec3f fract_point = fract3(x);

  double smoothness =
      1.0f +
      31.0f * pow(1.0f - v, 4.0f);  // Note: I use 31 instead of 63 (used in the
                                    // original paper) because the pow() behind
                                    // overflows with high numbers
  float va = 0.0f;
  float wt = 0.0f;
  for (int j = -2; j <= 2; j++)
    for (int i = -2; i <= 2; i++) {
      for (int k = -2; k <= 2; k++) {
        vec3f position = vec3f{float(k), float(i), float(j)};
        vec4f hashed   = hash4(floor_point + position) * vec4f{u, u, u, 1.0f};
        vec3f r = position - fract_point + vec3f{hashed.x, hashed.y, hashed.z};
        float d = length(r);
        long double w = pow(
            1.0 - smoothstep(0.0, 1.414, (double)d), smoothness);
        va += w * hashed.w;
        wt += w;
      }
    }

  return va / wt;
}
///////////////////////////// end of voronise

///////////////////////////// cell noise
float voronoiDistance(vec3f x) {
  vec3f p = floor3(x);
  vec3f f = fract3(x);

  vec3f mb;
  vec3f mr;

  float res = 8.0;
  for (int k = -1; k <= 1; k++) {
    for (int j = -1; j <= 1; j++)
      for (int i = -1; i <= 1; i++) {
        vec3f b    = vec3f{i, j, k};
        vec4f hash = hash4(p + b);
        vec3f r    = vec3f(b) + vec3f{hash.x, hash.y, hash.z} - f;
        float d    = dot(r, r);

        if (d < res) {
          res = d;
          mr  = r;
          mb  = b;
        }
      }
  }

  res = 8.0;
  for (int k = -2; k <= 2; k++) {
    for (int j = -2; j <= 2; j++) {
      for (int i = -2; i <= 2; i++) {
        vec3f b = mb + vec3f{i, j, k};
        vec3f r = vec3f(b) - f;
        float d = dot(0.5f * (mr + r), normalize(r - mr));

        res = min(res, d);
      }
    }
  }

  return res;
}

float getBorder(vec3f p) {
  float d = voronoiDistance(p);

  if (d > 0.05) return d;

  return d - smoothstep(0.0f, 0.05f, d);
}

//////////////////////////// smoothVoronoi

float smoothVoronoi(vec3f x) {
  vec3f p = floor3(x);
  vec3f f = fract3(x);

  float res = 0.0;
  for (int k = -1; k <= 1; k++) {
    for (int j = -1; j <= 1; j++)
      for (int i = -1; i <= 1; i++) {
        vec3f b    = vec3f{i, j, k};
        auto  hash = hash4(p + b);
        vec3f r    = vec3f(b) - f + vec3f{hash.x, hash.y, hash.z};
        float d    = dot(r, r);
        res += 1.0f / pow(d, 8.0f);
      }
  }
  return pow(1.0f / res, 1.0f / 16.0f);
}
//////////////////////////// smoothVoronoi

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

///////////////////////////// density for hair
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

    cdf[i] = w * (abs((density_map[t.x] + density_map[t.z] + density_map[t.y]) /
                      3)) +
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
  std::cout << shape.positions.size() << std::endl;
  for (int i = 0; i < shape.positions.size(); i++) {
    auto texture_value = eval_texture(
        scene, material.color_tex, shape.texcoords[i]);
    auto value = (texture_value.x + texture_value.y + texture_value.z) / 3;
    density_map.push_back(value);
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

///////////////////////////// end density for hair

void make_voro_terrain(shape_data& shape, const terrain_params& params) {
  float u = 1;
  float v = 1;
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = voronoise(pos * params.scale, u, v) * params.height;
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

void make_voro_displacement(
    shape_data& shape, const displacement_params& params) {
  float u = 0;
  float v = 0;
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = voronoise(pos * params.scale, u, v) * params.height;
    pos += norm * molt;

    // color
    auto height = molt / params.height;
    auto color  = height * params.top + (1 - height) * params.bottom;
    shape.colors.push_back(color);
  }
  // normals
  shape.normals = compute_normals(shape);
}

void make_smooth_voro_displacement(
    shape_data& shape, const displacement_params& params) {
  float u = 1;
  float v = 1;
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = smoothVoronoi(pos * params.scale) * params.height;
    pos += norm * molt;

    // color
    auto height = molt / params.height;
    auto color  = height * params.top + (1 - height) * params.bottom;
    shape.colors.push_back(color);
  }
  // normals
  shape.normals = compute_normals(shape);
}

void make_cell_voro_displacement(
    shape_data& shape, const displacement_params& params) {
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = voronoiDistance(pos * params.scale) * params.height;
    // pos += norm * molt;

    // color
    auto height = molt / params.height;
    auto color  = height * params.top + (1 - height) * params.bottom;
    shape.colors.push_back(vec4f{molt, molt, molt, 1});
  }
  // normals
  shape.normals = compute_normals(shape);
}

// I know, it's a ctrl+c ctrl+v, but i wanted to experiment how different
// noises interact and the result is pretty good, so i decided to keep it.
void make_world(shape_data& shape, const displacement_params& params) {
  float u = 1;
  float v = 1;
  for (int i = 0; i < shape.positions.size(); i++) {
    // position
    auto& pos  = shape.positions[i];
    auto& norm = shape.normals[i];
    auto  molt = (voronoise(pos * params.scale, u, v) +
                    fbm(pos * params.scale, 8) + ridge(pos * params.scale, 8)) *
                params.height;
    pos += norm * molt;

    // color
    auto height = molt / params.height;
    auto color  = height * params.top + (1 - height) * params.bottom;
    shape.colors.push_back(color);
  }
  // normals
  shape.normals = compute_normals(shape);
}

void make_displacement(shape_data& shape, const displacement_params& params) {
  make_voro_displacement(shape, params);
  return;
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
