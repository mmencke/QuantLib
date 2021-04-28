/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2021 Magnus Mencke

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

/*! \file midpointcdsengine.hpp
    \brief Mid-point engine for credit default swaps
*/

#ifndef quantlib_mc_cir_cds_option_engine_hpp
#define quantlib_mc_cir_cds_option_engine_hpp

#include <ql/experimental/credit/cdsoption.hpp>
#include <ql/models/shortrate/onefactormodels/coxingersollross.hpp>

namespace QuantLib {

    class McCirCdsOptionEngine : public CdsOption::engine {
      public:
        McCirCdsOptionEngine(ext::shared_ptr<CoxIngersollRoss> model,
          Real nSamples,
          Real seed,
                          Real recoveryRate,
                          Handle<YieldTermStructure> discountCurve,
                          const boost::optional<bool>& includeSettlementDateFlows = boost::none);
        void calculate() const override;

      private:
        ext::shared_ptr<CoxIngersollRoss> model_;
        Real nSamples_,seed_, recoveryRate_;
        Handle<YieldTermStructure> discountCurve_;
        boost::optional<bool> includeSettlementDateFlows_;
    };

}


#endif
