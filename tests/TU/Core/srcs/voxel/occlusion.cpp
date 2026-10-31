#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <future>
#include <vector>
#include "voxel/occlusion/cache.hpp"

namespace
{
using namespace spk::Voxel;
using namespace spk::Voxel::Occlusion;
using V = Shape::Vertex;

std::vector<V> face(float x, float low, float high, bool positive)
{
 std::vector<V> v{
  {{x,low,low},{0,0},spk::Vector3{positive?1.0f:-1.0f,0,0}},
  {{x,high,low},{1,0},spk::Vector3{positive?1.0f:-1.0f,0,0}},
  {{x,high,high},{1,1},spk::Vector3{positive?1.0f:-1.0f,0,0}},
  {{x,low,high},{0,1},spk::Vector3{positive?1.0f:-1.0f,0,0}}
 };
 if (!positive) std::reverse(v.begin(),v.end());
 return v;
}
Shape shape(float x, float low=0, float high=1, float opacity=1, bool positive=true)
{
 Shape::Builder b;
 b.addSlot("surface",opacity);
 b.addPolygon("surface",face(x,low,high,positive));
 return b.build();
}
Cache::Key key(const Shape &a,const Shape &b)
{
 Cache::Key k;
 k.source=a;
 k.blocker=b;
 k.direction=Shape::Face::PositiveX;
 return k;
}
float surface(const Result &result)
{
 float sum=0;
 for (const auto &p:result.fragments()) {
  const auto &v=p.vertices;
  for (std::size_t i=1;i+1<v.size();++i) {
   auto a=v[i].position-v[0].position;
   auto b=v[i+1].position-v[0].position;
   sum+=static_cast<float>(a.cross(b).length())*0.5f;
  }
 }
 return sum;
}
}

TEST(VoxelOcclusion, CompleteCoverage)
{
 auto a=shape(1),b=shape(0,0,1,1,false);
 EXPECT_TRUE(Algorithm::compute(a,{},b,{},Shape::Face::PositiveX).empty());
}
TEST(VoxelOcclusion, NoCoverage)
{
 auto a=shape(1),b=shape(0,0,0.25f,0,false);
 auto result=Algorithm::compute(a,{},b,{},Shape::Face::PositiveX);
 EXPECT_NEAR(surface(result),1.0f,1e-4f);
}
TEST(VoxelOcclusion, PartialCoverageAndUV)
{
 auto a=shape(1),b=shape(0,0,0.5f,1,false);
 auto result=Algorithm::compute(a,{},b,{},Shape::Face::PositiveX);
 EXPECT_NEAR(surface(result),0.75f,1e-4f);
 for (const auto &p:result.fragments())
  for (const auto &v:p.vertices) {
   EXPECT_NEAR(v.position.x,1.0f,1e-5f);
   EXPECT_GE(v.uv.x,-1e-4f);
   EXPECT_LE(v.uv.x,1.0001f);
   EXPECT_GE(v.uv.y,-1e-4f);
   EXPECT_LE(v.uv.y,1.0001f);
   ASSERT_TRUE(v.normal.has_value());
   EXPECT_NEAR(v.normal->length(),1.0,1e-4);
  }
}
TEST(VoxelOcclusion, WrongFacingBlockerDoesNotOcclude)
{
 auto a=shape(1),b=shape(0);
 EXPECT_NEAR(surface(Algorithm::compute(a,{},b,{},Shape::Face::PositiveX)),1.0f,1e-4f);
}
TEST(VoxelOcclusion, CacheGetAndClear)
{
 auto a=shape(1),b=shape(0,0,0.5f,1,false);
 Cache cache;
 auto k=key(a,b);
 auto first=cache.get(k);
 auto second=cache.get(k);
 EXPECT_EQ(&first.fragments(),&second.fragments());
 EXPECT_NEAR(surface(first),0.75f,1e-4f);
 cache.clear();
 auto third=cache.get(k);
 EXPECT_NE(&first.fragments(),&third.fragments());
 EXPECT_NEAR(surface(third),surface(first),1e-5f);
}
TEST(VoxelOcclusion, CacheKeyDirectionAndTolerance)
{
 auto a=shape(1),b=shape(0,0,0.5f,1,false);
 auto k=key(a,b);
 auto different=k;
 different.direction=Shape::Face::NegativeX;
 EXPECT_FALSE(k==different);
 different=k;
 different.tolerance=1e-4f;
 EXPECT_FALSE(k==different);
 different=k;
 different.sourceOrientation.horizontal=Shape::HorizontalOrientation::XNegative;
 EXPECT_FALSE(k==different);
}
TEST(VoxelOcclusion, ConcurrentCacheGet)
{
 auto a=shape(1),b=shape(0,0,0.5f,1,false);
 Cache cache;
 auto k=key(a,b);
 std::vector<std::future<Result>> jobs;
 for (int i=0;i<16;++i)
  jobs.push_back(std::async(std::launch::async,[&]{return cache.get(k);}));
 for (auto &job:jobs)
  EXPECT_NEAR(surface(job.get()),0.75f,1e-4f);
}
TEST(VoxelOcclusion, RejectInvalidTolerance)
{
 auto a=shape(1),b=shape(0,0,1,1,false);
 EXPECT_ANY_THROW(Algorithm::compute(a,{},b,{},Shape::Face::PositiveX,0));
 EXPECT_ANY_THROW(Algorithm::compute(a,{},b,{},Shape::Face::PositiveX,-1));
}
