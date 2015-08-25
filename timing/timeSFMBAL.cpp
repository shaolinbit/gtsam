/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    timeSFMBAL.cpp
 * @brief   time SFM with BAL file,  conventional GeneralSFMFactor
 * @author  Frank Dellaert
 * @date    June 6, 2015
 */

#include "timeSFMBAL.h"

#include <gtsam/slam/GeneralSFMFactor.h>
#include <gtsam/geometry/Cal3Bundler.h>
#include <gtsam/geometry/PinholeCamera.h>
#include <gtsam/geometry/Point3.h>

#include <boost/foreach.hpp>

using namespace std;
using namespace gtsam;

typedef PinholeCamera<Cal3Bundler> Camera;
typedef GeneralSFMFactor<Camera, Point3> SfmFactor;

int main(int argc, char* argv[]) {
  // parse options and read BAL file
  SfM_data db = preamble(argc, argv);

  // Build graph using conventional GeneralSFMFactor
  NonlinearFactorGraph graph;
  for (size_t j = 0; j < db.number_tracks(); j++) {
    BOOST_FOREACH (const SfM_Measurement& m, db.tracks[j].measurements) {
      size_t i = m.first;
      Point2 z = m.second;
      graph.push_back(SfmFactor(z, gNoiseModel, C(i), P(j)));
    }
  }

  Values initial;
  size_t i = 0, j = 0;
  BOOST_FOREACH (const SfM_Camera& camera, db.cameras)
    initial.insert(C(i++), camera);
  BOOST_FOREACH (const SfM_Track& track, db.tracks)
    initial.insert(P(j++), track.p);

  return optimize(db, graph, initial);
}