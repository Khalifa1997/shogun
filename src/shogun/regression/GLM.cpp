#include <shogun/lib/config.h>

#include <shogun/labels/RegressionLabels.h>
#include <shogun/lib/SGVector.h>
#include <shogun/mathematics/Math.h>
#include <shogun/mathematics/linalg/LinalgNamespace.h>
#include <shogun/mathematics/linalg/LinalgSpecialPurposes.h>

#include <shogun/regression/GLM.h>
#include <utility>
using namespace shogun;

GLM::GLM() : LinearMachine()
{
	init();
}
float64_t GLM::log_likelihood(
    const std::shared_ptr<DenseFeatures<float64_t>>& features,
    const std::shared_ptr<Labels>& label)
{
	auto vector_count = features->get_num_vectors();
	auto feature_count = features->get_num_features();
	ASSERT(vector_count > 0 && label->get_num_labels() == vector_count)

	SGVector<float64_t> lambda(vector_count);
	SGVector<float64_t> beta = LinearMachine::get_w();
	float64_t beta0 = LinearMachine::get_bias();
	for (auto i = 0; i < vector_count; i++)
	{
		SGVector<float64_t> feature_vector = features->get_feature_vector(i);
		float64_t res = linalg::dot(feature_vector, beta);
		lambda.set_element(log(1 + std::exp(beta0 + res)), i);
	}
	SGVector<float64_t> likelihood(vector_count);
	SGVector<float64_t> labels = label->get_values();
	SGVector<float64_t> log_lambda(vector_count);

	for (auto i = 0; i < vector_count; i++)
		log_lambda.set_element(log(lambda.get_element(i)), i);

	likelihood = linalg::add(
	    linalg::element_prod(labels, log_lambda), lambda, 1.0, -1.0);
	return linalg::sum(likelihood);
}

SGVector<float64_t> GLM::log_likelihood_derivative(
    const std::shared_ptr<DenseFeatures<float64_t>>& features,
    const std::shared_ptr<Labels>& label)
{
	auto vector_count = features->get_num_vectors();
	auto feature_count = features->get_num_features();
	ASSERT(vector_count > 0 && label->get_num_labels() == vector_count)
	SGVector<float64_t> result(vector_count + 1);
	SGVector<float64_t> beta = LinearMachine::get_w();
	float64_t beta0 = LinearMachine::get_bias();
	SGMatrix<float64_t> z = linalg::matrix_prod(
	    SGMatrix<float64_t>(beta, 1, beta.vlen),
	    features->get_feature_matrix()); // Z is 1xN Matrix where N is the
	                                     // number of vectors
	linalg::add_scalar(z, beta0);
	SGMatrix<float64_t> s(z.num_rows, z.num_cols);
	linalg::logistic(z, s);
	SGVector<float64_t> q(vector_count);
	for (auto i = 0; i < vector_count; i++)
	{
		q.set_element(log(1 + std::exp(z.get_element(0, i))), i);
	}
	float64_t beta0_grad =
	    linalg::sum(s) -
	    linalg::sum(linalg::element_prod(label->get_values(), SGVector(s)));
	result.set_element(beta0_grad, 0);
	return result;
}
void GLM::init()
{
	SG_ADD(
	    &m_tau, "tau", "L2 Regularization parameter",
	    ParameterProperties::SETTING);
	SG_ADD(
	    (std::shared_ptr<SGObject>*)&m_descend_updater, "descend_updater",
	    "Descend Updater used for updating weights",
	    ParameterProperties::SETTING);
	SG_ADD(
	    &m_family, "family", "Distribution Family used",
	    ParameterProperties::SETTING);
	SG_ADD(
	    &m_link_fn, "link_fn", "Link function used",
	    ParameterProperties::SETTING);
}
GLM::GLM(
    const std::shared_ptr<DescendUpdater>& descend_updater,
    DistributionFamily family, LinkFunction link_fn, float64_t tau)
    : LinearMachine()
{
	m_tau = tau;
	m_link_fn = link_fn;
	m_descend_updater = descend_updater;
	m_family = family;
	init();
}