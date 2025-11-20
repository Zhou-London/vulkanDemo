#pragma once
#include "config.h"

#include <memory>
#include "IModel.h"

class Sphere : public IModel {
 public:
  Sphere();
  Sphere(float radius, int sectorCount, int stackCount);

  ~Sphere() = default;

  void generate() override;

  static std::unique_ptr<IModel> make_sphere(float radius,
                                             int sectorCount,
                                             int stackCount) {
    return std::make_unique<Sphere>(radius, sectorCount, stackCount);
  }

 private:
  float radius_;
  int sectorCount_;
  int stackCount_;
};