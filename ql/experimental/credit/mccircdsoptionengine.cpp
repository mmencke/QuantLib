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

#include <ql/pricingengines/credit/midpointcdsengine.hpp>
#include <ql/experimental/credit/mccircdsoptionengine.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/methods/montecarlo/montecarlomodel.hpp>
#include <ql/math/interpolations/linearinterpolation.hpp>
#include <ql/termstructures/credit/interpolatedsurvivalprobabilitycurve.hpp>

#include <utility>

namespace QuantLib {

    McCirCdsOptionEngine::McCirCdsOptionEngine(ext::shared_ptr<CoxIngersollRoss> model,
      Real nSamples,
      Real seed,

                                         Real recoveryRate,
                                         Handle<YieldTermStructure> discountCurve,
                                         const boost::optional<bool>& includeSettlementDateFlows)
    : model_(model), nSamples_(nSamples), seed_(seed),recoveryRate_(recoveryRate),
      discountCurve_(std::move(discountCurve)),
      includeSettlementDateFlows_(includeSettlementDateFlows) {
        registerWith(discountCurve_);
    }

    void McCirCdsOptionEngine::calculate() const {
        ext::shared_ptr< CreditDefaultSwap > cds = arguments_.swap;
        Date todaysDate = Settings::instance().evaluationDate();
        Date startDate = todaysDate;
        Date exerciseDate = cds->protectionStartDate();
        Date maturityDate = cds->protectionEndDate();

        Size nTimeSteps = 1; //only one time step needed
        //This is only used for determinng how long to simulate
        Real t=Actual360().yearFraction(startDate,exerciseDate);
        Real T=Actual360().yearFraction(startDate,maturityDate);
        Real nCurveSteps=(T-t)*4; //we want 4 timesteps per year (once per quarter)
        Real dt=(T-t)/nCurveSteps;

        TimeGrid timeGrid(t,nTimeSteps);

        ext::shared_ptr<StochasticProcess1D> process = model_->dynamics()->process();

        PseudoRandom::rsg_type rsg = PseudoRandom::make_sequence_generator(nTimeSteps, seed_);
        ext::shared_ptr<SingleVariate<PseudoRandom>::path_generator_type> cirRsg(
          new SingleVariate<PseudoRandom>::path_generator_type(process, timeGrid, rsg,false));

        Real sum=0;
        Real npv;

        for(int i=0;i<nSamples_;i++) {
          Path path = (cirRsg->next()).value;
          Real cirValue =path[nTimeSteps];

          std::vector<Date> dates;
          std::vector<Probability> survivalProbabilities;
          dates.push_back(exerciseDate);
          survivalProbabilities.push_back(1);

          for(int j=1;j<=nCurveSteps;j++) {
            dates.push_back(exerciseDate+j*dt*365);
            survivalProbabilities.push_back(model_->discountBond(t,t+j*dt,cirValue));
          }
          //correct probabilities to avoid negative hazard rates
          for(int j=0;j<(survivalProbabilities.size()-1);j++) {
            if(survivalProbabilities[j+1]>survivalProbabilities[j]) {
              survivalProbabilities[j+1]=survivalProbabilities[j];
            }
          }

          ext::shared_ptr<InterpolatedSurvivalProbabilityCurve<Linear>> probabilityCurve(
            new InterpolatedSurvivalProbabilityCurve<Linear>(dates, survivalProbabilities, Actual360()));

          probabilityCurve->enableExtrapolation(true);

          Handle<DefaultProbabilityTermStructure> probability(probabilityCurve);

          ext::shared_ptr<MidPointCdsEngine> helpingEngine(
            new MidPointCdsEngine(probability,recoveryRate_,discountCurve_,includeSettlementDateFlows_));

          cds->setPricingEngine(helpingEngine);

          npv= cds->NPV();

          if(npv>0) {
            sum+=npv;
          }
         }
//might discount slightly wrong
        results_.value = sum/nSamples_;
      }
}
