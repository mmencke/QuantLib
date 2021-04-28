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


#include <ql/models/shortrate/calibrationhelpers/cdsoptionhelper.hpp>

#include <ql/pricingengines/credit/midpointcdsengine.hpp>
#include <ql/experimental/credit/blackcdsoptionengine.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/schedule.hpp>
#include <ql/exercise.hpp>
#include <utility>

namespace QuantLib {

    CdsOptionHelper::CdsOptionHelper(const Period& maturity,
                   const Period& length,
                   const Handle<Quote>& volatility,
                   Real recoveryRate,
                   Handle<DefaultProbabilityTermStructure> defaultProbabilityCurve,
                   Handle<YieldTermStructure> discountCurve,
                   BlackCalibrationHelper::CalibrationErrorType errorType,
                   Frequency paymentFrequency ,
                   Calendar calendar,
                   BusinessDayConvention paymentConvention,
                   BusinessDayConvention terminationDateConvention,
                   DateGeneration::Rule dateRule,
                   DayCounter dayCounter)
    : BlackCalibrationHelper(volatility, errorType),
    maturity_(maturity), length_(length), recoveryRate_(recoveryRate),
    defaultProbabilityCurve_(std::move(defaultProbabilityCurve)), discountCurve_(std::move(discountCurve)),
    paymentFrequency_(paymentFrequency), calendar_(calendar), paymentConvention_(paymentConvention),
    terminationDateConvention_(terminationDateConvention), dateRule_(dateRule), dayCounter_(dayCounter) {
        registerWith(defaultProbabilityCurve_);
        registerWith(discountCurve_);
    }


    Real CdsOptionHelper::modelValue() const {
        calculate();
        cdsOption_->setPricingEngine(engine_);
        return cdsOption_->NPV();
    }

    Real CdsOptionHelper::blackPrice(Volatility sigma) const {
        calculate();
        Handle<Quote> vol(ext::shared_ptr<Quote>(new SimpleQuote(sigma)));
        ext::shared_ptr<PricingEngine> engine;
        engine = ext::make_shared<BlackCdsOptionEngine>(
                defaultProbabilityCurve_, recoveryRate_, discountCurve_,vol);

        cdsOption_->setPricingEngine(engine);
        Real value = cdsOption_->NPV();
        cdsOption_->setPricingEngine(engine_);
        return value;
    }

    void CdsOptionHelper::performCalculations() const {
          Date startDate =calendar_.advance(Settings::instance().evaluationDate(), maturity_);
          Date endDate =calendar_.advance(startDate,length_);

          Schedule cdsSchedule = MakeSchedule()
          .from(startDate)
          .to(endDate)
          .withFrequency(paymentFrequency_)
          .withCalendar(calendar_)
          .withConvention(paymentConvention_)
          .withTerminationDateConvention(terminationDateConvention_)
          .withRule(dateRule_);

          Protection::Side side = Protection::Seller; //ATM put = ATM call

          //only running spread. 0.02 does not mean anything. We don't use this, but need to define it to find the fair spread.
          ext::shared_ptr<CreditDefaultSwap> temp(new CreditDefaultSwap(side, 1, 0.02, cdsSchedule,paymentConvention_,dayCounter_));

          ext::shared_ptr<PricingEngine> cdsEngine(new MidPointCdsEngine(defaultProbabilityCurve_, recoveryRate_, discountCurve_));

          temp->setPricingEngine(cdsEngine);

          //only at the money
          Real forwardSpread =temp->fairSpread();

          cds_=ext::make_shared<CreditDefaultSwap>(side, 1, forwardSpread, cdsSchedule,paymentConvention_,dayCounter_);

          cds_->setPricingEngine(cdsEngine);

          ext::shared_ptr<EuropeanExercise> exercise(new EuropeanExercise(cds_->protectionStartDate()));

          cdsOption_=ext::make_shared<CdsOption>(cds_, exercise);

        BlackCalibrationHelper::performCalculations();

    }

}
