#ifndef quantlib_cir_helper_process_hpp
#define quantlib_cir_helper_process_hpp

#include <ql/stochasticprocess.hpp>
#include <ql/processes/eulerdiscretization.hpp>


namespace QuantLib {
class CirHelperProcess : public StochasticProcess1D {
  public:
    CirHelperProcess(Real theta, Real k, Real sigma, Real y0)
    : y0_(y0), theta_(theta), k_(k), sigma_(sigma) {
        discretization_ =
            ext::shared_ptr<discretization>(new EulerDiscretization);
    }

    Real x0() const override { return y0_; }
    Real drift(Time, Real y) const override {
      Real drift_adj;
      if(std::fabs(y)<0.001) {
        drift_adj=0; //make sure that the process does not explode when y approaches 0
      } else {
        drift_adj=(0.5*theta_*k_ - 0.125*sigma_*sigma_)/y
            - 0.5*k_*y;
      }
        return drift_adj;
    }
    Real diffusion(Time, Real) const override { return 0.5 * sigma_; }

  private:
    Real y0_, theta_, k_, sigma_;
};
}

#endif
