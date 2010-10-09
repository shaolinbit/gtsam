// These are currently broken
// Solve by parsing a namespace pose2SLAM::Values and making a Pose2SLAMValues class
// We also have to solve the shared pointer mess to avoid duplicate methods

class Pose2Values{
	Pose2Values();
	Pose2 get(string key) const;
	void insert(string name, const Pose2& val);
    void print(string s) const;
    void clear();
    int size();
};

class Pose2Factor {
	Pose2Factor(string key1, string key2,
			const Pose2& measured, Matrix measurement_covariance);
	void print(string name) const;
	double error(const Pose2Values& c) const;
	size_t size() const;
	GaussianFactor* linearize(const Pose2Values& config) const;
};

class Pose2Graph{
	Pose2Graph();
	void print(string s) const;
	GaussianFactorGraph* linearize_(const Pose2Values& config) const;
	void push_back(Pose2Factor* factor);
};


