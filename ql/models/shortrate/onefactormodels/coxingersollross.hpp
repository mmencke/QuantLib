/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2001, 2002, 2003 Sadruddin Rejeb

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file coxingersollross.hpp
    \brief Cox-Ingersoll-Ross model
*/

#ifndef quantlib_cox_ingersoll_ross_hpp
#define quantlib_cox_ingersoll_ross_hpp

#include <ql/models/shortrate/onefactormodel.hpp>
#include <ql/processes/coxingersollrossprocess.hpp>

namespace QuantLib {

    //! Cox-Ingersoll-Ross model class.
    /*! This class implements the Cox-Ingersoll-Ross model defined by
        \f[
            dr_t = k(\theta - r_t)dt + \sqrt{r_t}\sigma dW_t .
        \f]

        \bug this class was not tested enough to guarantee
             its functionality.

        \ingroup shortrate
    */
    class CoxIngersollRoss : public OneFactorAffineModel {
      public:
        CoxIngersollRoss(Rate r0 = 0.05,
                         Real theta = 0.1,
                         Real k = 0.1,
                         Real sigma = 0.1,
                         bool withFellerConstraint = true,
                         CoxIngersollRossProcess::Discretization d = CoxIngersollRossProcess::QuadraticExponential);

        Real discountBondOption(Option::Type type,
                                Real strike,
                                Time maturity,
                                Time bondMaturity) const override;

        ext::shared_ptr<ShortRateDynamics> dynamics() const override;

        ext::shared_ptr<Lattice> tree(const TimeGrid& grid) const override;


        class Dynamics;
      protected:
        Real A(Time t, Time T) const override;
        Real B(Time t, Time T) const override;

        Real theta() const { return theta_(0.0); }
        Real k() const { return k_(0.0); }
        Real sigma() const { return sigma_(0.0); }
        Real x0() const { return r0_(0.0); }

        CoxIngersollRossProcess::Discretization discretization() const {return discretization_;}

      private:
        class VolatilityConstraint;

        Parameter& theta_;
        Parameter& k_;
        Parameter& sigma_;
        Parameter& r0_;
        CoxIngersollRossProcess::Discretization discretization_;
    };

    //! %Dynamics of the short-rate under the Cox-Ingersoll-Ross model
    /*! The state variable \f$ y_t \f$ will here be the square-root of the
        short-rate. It satisfies the following stochastic equation
        \f[
            dy_t=\left[
                    (\frac{k\theta }{2}+\frac{\sigma ^2}{8})\frac{1}{y_t}-
                    \frac{k}{2}y_t \right] d_t+ \frac{\sigma }{2}dW_{t}
        \f].
    */
    class CoxIngersollRoss::Dynamics :
        public OneFactorModel::ShortRateDynamics {
      public:
        Dynamics(Real theta,
                 Real k,
                 Real sigma,
                 Real x0,
                 CoxIngersollRossProcess::Discretization d)
        : ShortRateDynamics(ext::shared_ptr<StochasticProcess1D>(
                        new CoxIngersollRossProcess(k, sigma, x0, theta, d))) {}

        Real variable(Time, Rate r) const override { return r; }
        Real shortRate(Time, Real y) const override { return y; }
    };

}


#endif
