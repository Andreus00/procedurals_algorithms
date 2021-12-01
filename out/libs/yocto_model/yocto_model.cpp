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

#include <algorithm>
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
  return vec4f{floorf(x.x), floorf(x.y), floorf(x.z), floorf(x.w)};
}
vec3f floor3(vec3f x) { return vec3f{floorf(x.x), floorf(x.y), floorf(x.z)}; }
vec4f fract4(vec4f x) { return x - floor4(x); }
vec3f fract3(vec3f x) { return x - floor3(x); }

vec4f hash4(vec3f p) {
  vec4f q = vec4f{dot(p, vec3f{127.1, 311.7, 411.3}),
      dot(p, vec3f{269.5, 183.3, 152.4}), dot(p, vec3f{419.2, 371.9, 441.0}),
      dot(p, vec3f{512.4, 321.9, 69.420})};
  return fract4(sin(q) * 43758.5453);
}

float voronoise(vec3f x, float u, float v) {
  vec3f floor_point = floor3(x);
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
  for (float k = -1; k <= 1; k++) {
    for (float j = -1; j <= 1; j++)
      for (float i = -1; i <= 1; i++) {
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
  for (float k = -2; k <= 2; k++) {
    for (float j = -2; j <= 2; j++) {
      for (float i = -2; i <= 2; i++) {
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

  return d * (smoothstep(0.0f, 0.05f, d));
}

//////////////////////////// smoothVoronoi

float smoothVoronoi(vec3f x) {
  vec3f p = floor3(x);
  vec3f f = fract3(x);

  float res = 0.0;
  for (float k = -1; k <= 1; k++) {
    for (float j = -1; j <= 1; j++)
      for (float i = -1; i <= 1; i++) {
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

struct my_entry {
  float* weight;
  int    position;

  inline bool operator<(const my_entry& b) const {
    return (*weight) < (*b.weight);
  }
};

bool compare_entry(const my_entry& a, const my_entry& b) {
  return (*a.weight) < (*b.weight);
}

void sample_elimination(vector<vec3f>& positions, vector<vec3f>& normals,
    vector<vec2f>& texcoords, float cell_size, float influence_radius,
    int desired_samples) {
  // Create the grid
  const float ALPHA = 8.0f;
  std::cout << "creating the hashgrid" << std::endl;
  auto grid = make_hash_grid(positions, cell_size);

  // assign weight to each position
  std::cout << "weight calculation" << std::endl;
  vector<float> weight;
  for (int i = 0; i < grid.positions.size(); i++) {
    auto        current_point = grid.positions[i];
    vector<int> neighbors;
    find_neighbors(grid, neighbors, i, influence_radius);
    auto sum = 0.0f;
    for (auto& neighbor_idx : neighbors) {
      auto neighbor = grid.positions[neighbor_idx];
      sum += pow(
          1.0f - distance(neighbor, current_point) / (2.0f * influence_radius),
          ALPHA);
    }
    weight.push_back(sum);
  }

  // adding the entry to a list that will be converted in a heap
  std::cout << "building the heap" << std::endl;
  vector<my_entry> heap_weight_positions;
  for (int i = 0; i < weight.size(); i++) {
    struct my_entry e = {&weight[i], i};
    heap_weight_positions.push_back(e);
  }

  // building the heap based on the weight

  make_heap(heap_weight_positions.begin(), heap_weight_positions.end(),
      compare_entry);
  // while there are more samples than the required ones
  auto counter = heap_weight_positions.size() - 1;
  std::cout << "sample elimination" << std::endl;
  while (counter > desired_samples) {
    if ((counter % 100) == 0)
      std::cout << counter << " / " << desired_samples << std::endl;
    // get the root of the heap and remove it
    auto element          = heap_weight_positions[0];
    auto eliminated_point = grid.positions[(element.position)];
    pop_heap(
        heap_weight_positions.begin(), heap_weight_positions.begin() + counter);

    // get the neighbors of the removed position
    vector<int> neighbors;
    find_neighbors(grid, neighbors, element.position, influence_radius);
    for (auto neighbor_index : neighbors) {
      // foreach neighbor, I remove the weight of the removed position from the
      // total weight of the neighbor
      auto neighbor = grid.positions[neighbor_index];
      auto dist     = distance(neighbor, eliminated_point);
      weight[neighbor_index] -= pow(
          1.0f -
              distance(neighbor, eliminated_point) / (2.0f * influence_radius),
          ALPHA);
    }
    counter--;
    // rebuild the heap
    make_heap(heap_weight_positions.begin(),
        heap_weight_positions.begin() + counter, compare_entry);
  }

  std::cout << "writing the solution" << std::endl;
  // write the solution

  vector<vec3f> new_pos;
  vector<vec3f> new_norm;
  vector<vec2f> new_texcoord;
  for (int i = 0; i < desired_samples; i++) {
    auto el = heap_weight_positions[i];
    new_pos.push_back(positions[(el.position)]);
    new_norm.push_back(normals[(el.position)]);
    new_texcoord.push_back(texcoords[(el.position)]);
  }
  for (int i = 0; i < desired_samples; i++) {
    positions[i] = new_pos[i];
    normals[i]   = new_norm[i];
    texcoords[i] = new_texcoord[i];
  }
  while (positions.size() > desired_samples) {
    positions.pop_back();
    normals.pop_back();
    texcoords.pop_back();
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
    shape_data& shape, const displacement_params& params, float u, float v) {
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
    auto  molt = getBorder(pos * params.scale) * params.height;
    pos += norm * molt;

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

void make_hair_sample_elimination(
    shape_data& hair, const shape_data& shape, const hair_params& params) {
  auto          segment_length = params.lenght / params.steps;
  vector<vec3f> positions;
  vector<vec3f> normals;
  vector<vec2f> texcoords;
  sample_shape(positions, normals, texcoords, shape, params.num * 5);
  sample_elimination(positions, normals, texcoords, params.cell_size,
      params.influence_radius, params.num);
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

////////////////////////////////////// Trees

void addChild(struct Branch* parent, int child) {
  parent->_children.push_back(child);
}
vector<int>   getChildren(struct Branch* parent) { return parent->_children; }
vector<vec3f> getAttractors(struct Branch* parent) {
  return parent->_attractors;
}

void init_branch(struct Branch* b, vec3f start, vec3f end, vec3f direction,
    int parent, float thickness) {
  b->start        = start;
  b->end          = end;
  b->direction    = direction;
  b->parent_index = parent;
  b->thickness    = thickness;
}

void draw_branch(scene_data& scene, struct Branch* b, int shape, int material) {
  instance_data inst;
  inst.material = material;
  inst.shape    = shape;

  inst.frame = frame_fromz(
      interpolate_line(b->start, b->end, 0.5), b->direction);

  scene.instances.push_back(inst);
}

void leafs_distribution(vector<vec3f>* out, vec3f base, float crown_radius,
    int number, float height) {
  auto rng = make_rng(666);
  for (int i = 0; i < number; i++) {
    vec3f rand = rand3f(rng);
    vec3f D    = {cos(rand[0] * 2.0f * pif) * sin(rand[1] * 2.0f * pif),
        sin(rand[0] * 2.0f * pif) * sin(rand[1] * 2.0f * pif),
        cos(rand[1] * 2.0f * pif)};
    vec3f sampled_point = D * (rand[2] * crown_radius);
    out->push_back(sampled_point + base + vec3f{0, height, 0});
  }
}

bool is_in_range(vec3f p, float radius, vec3f base, float height) {
  auto point_center = distance(p, base + vec3f{0, height, 0});
  return point_center < radius;
}

void generate_tree(scene_data& scene, const vec3f start, const vec3f norm,
    const tree_params& params) {
  auto rng = make_rng(777);
  // create the first branch
  struct Branch first_branch;
  init_branch(&first_branch, start, start + norm * params.step_len, norm, 0,
      params.thickness);
  // sampling the points for the crown of the tree
  vector<vec3f> crown_points;
  leafs_distribution(&crown_points, first_branch.start, params.crown_radius,
      params.leaves_num * 4, params.crown_height);

  // Useless vecors. I need them for the sample elimination call.
  vector<vec3f> normals(params.leaves_num * 4, zero3f);
  vector<vec2f> texcoord(params.leaves_num * 4, zero2f);
  sample_elimination(crown_points, normals, texcoord,
      params.crown_points_distance * 0.4, params.crown_points_distance * 0.8,
      params.leaves_num);

  // draw the points for the visualization
  auto          sphere = make_sphere(32, 0.01f);
  material_data sphere_material;
  sphere_material.color = {1, 0, 0};
  sphere_material.type  = material_type::matte;
  auto sphere_shape_idx = scene.shapes.size();
  scene.shapes.push_back(sphere);
  auto sphere_material_idx = scene.materials.size();
  scene.materials.push_back(sphere_material);

  // simulate the growth of the tree

  // create the cilinder
  auto cilinder = make_uvcylinder(vec3i{32, 32, 32},
      vec2f{params.thickness, params.step_len / 2}, vec3f{(1), (1), (1)});

  scene.shapes.push_back(cilinder);
  auto cilinder_index = scene.shapes.size() - 1;

  material_data segment_material;
  segment_material.color = {1, 0, 1};
  segment_material.type  = material_type::matte;
  scene.materials.push_back(segment_material);
  auto material_index = scene.materials.size() - 1;

  vector<struct Branch> branches;
  branches.push_back(first_branch);
  int queue_start   = 0;
  int division_flag = 0;
  for (int i = 0; i < params.steps; i++) {
    struct Branch current;
    struct Branch parent = branches[queue_start];
    if (queue_start > branches.size() - 1 || crown_points.size() == 0) break;
    if (division_flag > 2) {
      if (!is_in_range(parent.start, params.crown_radius, first_branch.start,
              params.crown_height)) {
        queue_start++;
        continue;
      }
    }

    auto  randomness = rand3f(rng);
    vec3f norm       = normalize(parent.direction * (randomness);
    vec3f cur_start  = parent.end;
    int   forks      = 0;
    // trovo i punti vicini
    auto sum = zero3f;
    for (int i = crown_points.size() - 1; i >= 0; i--) {
      auto p                            = crown_points[i];
      auto dist                         = distance(p, cur_start);
      auto curstart_to_p                = normalize(p - cur_start);
      auto dot_curdirection_curstarttop = dot(curstart_to_p, parent.direction);
      std::cout << dot_curdirection_curstarttop << std::endl;
      if (dist < params.range &&
          dot_curdirection_curstarttop > params.ignore_points_behind) {
        sum += curstart_to_p;
        if (dist < params.kill_range) {
          crown_points.erase(crown_points.begin() + i);
          forks++;
        }
      }
    }
    norm = normalize((norm + sum));
    if (forks > 0) {
      if (rand1f(rng) < params.fork_chance) {
        auto fork_norm = normalize(
            reflect(parent.direction, norm) * rand3f(rng));
        struct Branch fork_branch;
        init_branch(&fork_branch, cur_start,
            cur_start + fork_norm * params.step_len, fork_norm, queue_start,
            parent.thickness * params.thickness_decrease);
        branches.push_back(fork_branch);
        draw_branch(scene, &fork_branch, cilinder_index, material_index);
        division_flag++;
        addChild(&parent, branches.size() - 1);
      }
    }
    //  cercolo il punto finale del branch
    vec3f cur_end = cur_start + norm * params.step_len;

    // std::cout << norm.x << " " << norm.y << " " << norm.z << " " <<
    // std::endl;
    init_branch(&current, cur_start, cur_end, norm, queue_start,
        parent.thickness * params.thickness_decrease);
    branches.push_back(current);
    queue_start++;

    draw_branch(scene, &current, cilinder_index, material_index);
    addChild(&parent, branches.size() - 1);
  }

  for (auto& el : crown_points) {
    instance_data new_point;
    new_point.shape    = sphere_shape_idx;
    new_point.material = sphere_material_idx;
    new_point.frame    = frame3f{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, el};
    scene.instances.push_back(new_point);
  }

  material_data ray_sphere;
  ray_sphere.color = {0.8, 0.8, 0.8};
  ray_sphere.type  = material_type::transparent;
  scene.materials.push_back(ray_sphere);
  auto          ray_sphere_index = scene.materials.size() - 1;
  instance_data new_ray_sphere;
  new_ray_sphere.frame = frame3f{
      {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, branches.back().end};
  new_ray_sphere.material = ray_sphere_index;
  auto ray_sphere_sh      = make_sphere(32, params.range);
  scene.shapes.push_back(ray_sphere_sh);
  new_ray_sphere.shape = scene.shapes.size() - 1;
  scene.instances.push_back(new_ray_sphere);
}

}  // namespace yocto
