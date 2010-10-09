/** 
 * @file    testNonlinearFactorGraph.cpp
 * @brief   Unit tests for Non-Linear Factor Graph
 * @brief   testNonlinearFactorGraph
 * @author  Carlos Nieto
 * @author  Christian Potthast
 */

/*STL/C++*/
#include <iostream>
using namespace std;

#include <boost/assign/std/list.hpp>
using namespace boost::assign;

#include <gtsam/CppUnitLite/TestHarness.h>

#define GTSAM_MAGIC_KEY

#include <gtsam/base/Matrix.h>
#include <gtsam/slam/smallExample.h>
#include <gtsam/inference/FactorGraph-inl.h>
#include <gtsam/nonlinear/NonlinearFactorGraph-inl.h>

using namespace gtsam;
using namespace example;

/* ************************************************************************* */
TEST( Graph, equals )
{
	Graph fg = createNonlinearFactorGraph();
	Graph fg2 = createNonlinearFactorGraph();
	CHECK( fg.equals(fg2) );
}

/* ************************************************************************* */
TEST( Graph, error )
{
	Graph fg = createNonlinearFactorGraph();
	Values c1 = createValues();
	double actual1 = fg.error(c1);
	DOUBLES_EQUAL( 0.0, actual1, 1e-9 );

	Values c2 = createNoisyValues();
	double actual2 = fg.error(c2);
	DOUBLES_EQUAL( 5.625, actual2, 1e-9 );
}

/* ************************************************************************* */
TEST( Graph, GET_ORDERING)
{
  Ordering expected; expected += "x1","l1","x2";
  Graph nlfg = createNonlinearFactorGraph();
  Ordering actual = *nlfg.orderingCOLAMD(createNoisyValues()).first;
  CHECK(assert_equal(expected,actual));
}

/* ************************************************************************* */
TEST( Graph, probPrime )
{
	Graph fg = createNonlinearFactorGraph();
	Values cfg = createValues();

	// evaluate the probability of the factor graph
	double actual = fg.probPrime(cfg);
	double expected = 1.0;
	DOUBLES_EQUAL(expected,actual,0);
}

/* ************************************************************************* */
TEST( Graph, linearize )
{
	Graph fg = createNonlinearFactorGraph();
	Values initial = createNoisyValues();
	boost::shared_ptr<GaussianFactorGraph> linearized = fg.linearize(initial, *initial.orderingArbitrary());
	GaussianFactorGraph expected = createGaussianFactorGraph(*initial.orderingArbitrary());
	CHECK(assert_equal(expected,*linearized)); // Needs correct linearizations
}

/* ************************************************************************* */
int main() {
	TestResult tr;
	return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
