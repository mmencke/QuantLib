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

/*! \file CdsOptionHelper.hpp
    \brief Swaption calibration helper
*/

#ifndef quantlib_cdsoption_calibration_helper_hpp
#define quantlib_cdsoption_calibration_helper_hpp

#include <ql/models/calibrationhelper.hpp>
#include <ql/experimental/credit/cdsoption.hpp>

#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual360.hpp>


namespace QuantLib {

    //! calibration helper for ATM swaption

    class CdsOptionHelper : public BlackCalibrationHelper {
      public:
        CdsOptionHelper(const Period& maturity,
                       const Period& length,
                       const Handle<Quote>& volatility,
                       Real recoveryRate,
                       Handle<DefaultProbabilityTermStructure> defaultProbabilityCurve,
                       Handle<YieldTermStructure> discountCurve,
                       BlackCalibrationHelper::CalibrationErrorType errorType = BlackCalibrationHelper::RelativePriceError,
                       Frequency paymentFrequency = Quarterly,
                       Calendar calendar=TARGET(),
                       BusinessDayConvention paymentConvention =Following,
                       BusinessDayConvention terminationDateConvention=Unadjusted,
                       DateGeneration::Rule dateRule=DateGeneration::CDS2015,
                       DayCounter dayCounter = Actual360());

        void addTimesTo(std::list<Time>& times) const override {} //not implemented
        Real modelValue() const override;
        Real blackPrice(Volatility volatility) const override;

        ext::shared_ptr<CreditDefaultSwap> underlyingCds() const { calculate(); return cds_; }
        ext::shared_ptr<CdsOption> cdsOption() const { calculate(); return cdsOption_; }

      private:
        void performCalculations() const override;
        const Period maturity_, length_;
        const Real recoveryRate_;
        const Handle<DefaultProbabilityTermStructure> defaultProbabilityCurve_;
        const Handle<YieldTermStructure> discountCurve_;
        const Frequency paymentFrequency_;
        const Calendar calendar_;
        const BusinessDayConvention paymentConvention_, terminationDateConvention_;
        const DateGeneration::Rule dateRule_;
        const DayCounter dayCounter_;
        mutable ext::shared_ptr<CreditDefaultSwap> cds_;
        mutable ext::shared_ptr<CdsOption> cdsOption_;
    };

}

#endif
