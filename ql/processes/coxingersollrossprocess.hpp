/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2020 Lew Wei Hao

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
    \brief CoxIngersollRoss process
*/

#ifndef quantlib_coxingersollross_process_hpp
#define quantlib_coxingersollross_process_hpp

#include <iostream>

#include <ql/stochasticprocess.hpp>
#include <ql/math/distributions/normaldistribution.hpp>
#include <ql/math/distributions/chisquaredistribution.hpp>

namespace QuantLib {

    //! CoxIngersollRoss process class
    /*! This class describes the CoxIngersollRoss process governed by
        \f[
            dx = a (r - x_t) dt + \sqrt{x_t}\sigma dW_t.
        \f]

        \ingroup processes
    */
    class CoxIngersollRossProcess : public StochasticProcess1D {
      public:
        enum Discretization { None,
            FullTruncation,
            QuadraticExponentialMartingale,
            Exact
        };
        CoxIngersollRossProcess(Real speed,
                                 Volatility vol,
                                 Real x0 = 0.0,
                                 Real level = 0.0,
                                Discretization d = None);
        //@{
        Real drift(Time t, Real x) const override;
        Real diffusion(Time t, Real x) const override;
        Real expectation(Time t0, Real x0, Time dt) const override;
        Real stdDeviation(Time t0, Real x0, Time dt) const override;
        //@}
        Real x0() const override;
        Real speed() const;
        Real volatility() const;
        Real level() const;
        Real variance(Time t0, Real x0, Time dt) const override;
        Real evolve (Time t0,
                     Real x0,
                     Time dt,
                     Real dw) const override;
      private:
        Real x0_, speed_, level_;
        Volatility volatility_;
        Discretization discretization_;
    };

    // inline

    inline Real CoxIngersollRossProcess::x0() const {
        return x0_;
    }

    inline Real CoxIngersollRossProcess::speed() const {
        return speed_;
    }

    inline Real CoxIngersollRossProcess::volatility() const {
        return volatility_;
    }

    inline Real CoxIngersollRossProcess::level() const {
        return level_;
    }

    inline Real CoxIngersollRossProcess::drift(Time, Real x) const {
        return speed_ * (level_ - x);
    }

    inline Real CoxIngersollRossProcess::diffusion(Time, Real) const {
        return volatility_;
    }

    inline Real CoxIngersollRossProcess::expectation(Time, Real x0,
                                               Time dt) const {
        return level_ + (x0 - level_) * std::exp(-speed_*dt);
    }

    inline Real CoxIngersollRossProcess::stdDeviation(Time t, Real x0,
                                                Time dt) const {
        return std::sqrt(variance(t,x0,dt));
    }
    //IMPLEMENTED BY ME
    //Full truncation scheme (as in Brigo, Morini and Pallavicini)
    //Inspired by Heston implementation
    //  see Lord, R., R. Koekkoek and D. van Dijk (2006),
    // "A Comparison of biased simulation schemes for
    //  stochastic volatility models",
    // Working Paper, Tinbergen Institute
    inline Real CoxIngersollRossProcess::evolve (Time t0,
                                    Real x0,
                                    Time dt,
                                    Real dw) const {
        Real resultTrunc;
        switch (discretization_) {
            case None: {
                resultTrunc=apply(expectation(t0,x0,dt),stdDeviation(t0,x0,dt)*dw);
                break;
            }
            case FullTruncation: {
                Real x0_trunc = x0>0.0 ? x0 : 0.0;

                Real result = apply( expectation(t0, x0_trunc, dt),stdDeviation(t0,x0_trunc,dt)*dw);

                resultTrunc = result>0.0 ? result : 0.0;

                break;
            }
            case QuadraticExponentialMartingale: {
                const Real rho_=0;
                // for details of the quadratic exponential discretization scheme
                // see Leif Andersen,
                // Efficient Simulation of the Heston Stochastic Volatility Model
                const Real ex = std::exp(-speed_*dt);

                const Real m  =  level_+(x0-level_)*ex;
                const Real s2 =  x0*volatility_*volatility_*ex/speed_*(1-ex)
                               + level_*volatility_*volatility_/(2*speed_)*(1-ex)*(1-ex);
                const Real psi = s2/(m*m);

                const Real g1 =  0.5;
                const Real g2 =  0.5;
                      Real k0 = -rho_*speed_*level_*dt/volatility_;
                const Real k1 =  g1*dt*(speed_*rho_/volatility_-0.5)-rho_/volatility_;
                const Real k2 =  g2*dt*(speed_*rho_/volatility_-0.5)+rho_/volatility_;
                const Real k3 =  g1*dt*(1-rho_*rho_);
                const Real k4 =  g2*dt*(1-rho_*rho_);
                const Real A  =  k2+0.5*k4;

                if (psi < 1.5) {
                    const Real b2 = 2/psi-1+std::sqrt(2/psi*(2/psi-1));
                    const Real b  = std::sqrt(b2);
                    const Real a  = m/(1+b2);

                    if (discretization_ == QuadraticExponentialMartingale) {
                        // martingale correction
                        QL_REQUIRE(A < 1/(2*a), "illegal value");
                        k0 = -A*b2*a/(1-2*A*a)+0.5*std::log(1-2*A*a)
                             -(k1+0.5*k3)*x0;
                    }
                    resultTrunc = a*(b+dw)*(b+dw);
                }
                else {
                    const Real p = (psi-1)/(psi+1);
                    const Real beta = (1-p)/m;

                    const Real u = CumulativeNormalDistribution()(dw);

                    if (discretization_ == QuadraticExponentialMartingale) {
                        // martingale correction
                        QL_REQUIRE(A < beta, "illegal value");
                        k0 = -std::log(p+beta*(1-p)/(beta-A))-(k1+0.5*k3)*x0;
                    }
                    resultTrunc = ((u <= p) ? 0.0 : std::log((1-p)/(1-u))/beta);
              }
              break;
            }
            case Exact: {
              CumulativeNormalDistribution dwDist; //despite the name, dw is standard normal
              Real uniform = dwDist(dw); //transforming normal to uniform

              Real c=(4*speed_)/(volatility_*volatility_*(1-std::exp(-speed_*dt)));
              Real nu=(4*speed_*level_)/(volatility_*volatility_);
              Real eta=c*x0*std::exp(-speed_*dt);

              InverseNonCentralCumulativeChiSquareDistribution chi2(nu, eta,100);

              resultTrunc = chi2(uniform)/c;
            }
        }

        return resultTrunc;
      }

}

#endif
