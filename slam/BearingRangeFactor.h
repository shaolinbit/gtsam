/*
 * BearingRangeFactor.h
 *
 *   Created on: Apr 1, 2010
 *       Author: Kai Ni
 *  Description: a single factor contains both the bearing and the range to prevent handle to pair bearing and range factors
 */

#pragma once

#include <gtsam/slam/BearingFactor.h>
#include <gtsam/slam/RangeFactor.h>

namespace gtsam {

	/**
	 * Binary factor for a bearing measurement
	 */
	template<class Values, class PoseKey, class PointKey>
	class BearingRangeFactor: public NonlinearFactor2<Values, PoseKey, PointKey> {
	private:

		// the measurement
		Rot2 bearing_;
		double range_;

		typedef NonlinearFactor2<Values, PoseKey, PointKey> Base;

	public:

		BearingRangeFactor(); /* Default constructor */
		BearingRangeFactor(const PoseKey& i, const PointKey& j, const Rot2& bearing, const double range,
				const SharedGaussian& model) :
					Base(model, i, j), bearing_(bearing), range_(range) {
		}

		/** h(x)-z -> between(z,h(x)) for Rot2 manifold */
		Vector evaluateError(const Pose2& pose, const Point2& point,
				boost::optional<Matrix&> H1, boost::optional<Matrix&> H2) const {
			Matrix H11, H21, H12, H22;
			boost::optional<Matrix&> H11_ = H1 ? boost::optional<Matrix&>(H11) : boost::optional<Matrix&>();
			boost::optional<Matrix&> H21_ = H1 ? boost::optional<Matrix&>(H21) : boost::optional<Matrix&>();
			boost::optional<Matrix&> H12_ = H2 ? boost::optional<Matrix&>(H12) : boost::optional<Matrix&>();
			boost::optional<Matrix&> H22_ = H2 ? boost::optional<Matrix&>(H22) : boost::optional<Matrix&>();

			Rot2 y1 = pose.bearing(point, H11_, H12_);
			Vector e1 = Rot2::Logmap(bearing_.between(y1));

			double y2 = pose.range(point, H21_, H22_);
			Vector e2 = Vector_(1, y2 - range_);

			if (H1) *H1 = gtsam::stack(2, &H11, &H21);
			if (H2) *H2 = gtsam::stack(2, &H12, &H22);
			return concatVectors(2, &e1, &e2);
		}

		/** return the measured */
		inline const std::pair<Rot2, double> measured() const {
			return std::make_pair(bearing_, range_);
		}
	}; // BearingRangeFactor

} // namespace gtsam
