/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2001, 2002, 2003 Sadruddin Rejeb
 Copyright (C) 2005 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/reference/license.html>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file model.hpp
    \brief Abstract interest rate model class
*/

#ifndef quantlib_interest_rate_model_hpp
#define quantlib_interest_rate_model_hpp

#include <ql/option.hpp>
#include <ql/methods/lattices/lattice.hpp>
#include <ql/models/shortrate/parameter.hpp>
#include <ql/models/shortrate/calibrationhelper.hpp>

namespace QuantLib {

    class OptimizationMethod;
    class EndCriteria;

    //! Affine model class
    /*! Base class for analytically tractable models.

        \ingroup shortrate
    */
    class AffineModel : public virtual Observable {
      public:
        //! Implied discount curve
        virtual DiscountFactor discount(Time t) const = 0;

        virtual Real discountBond(Time now,
                                    Time maturity,
                                    Array factors) const = 0;

        virtual Real discountBondOption(Option::Type type,
                                        Real strike,
                                        Time maturity,
                                        Time bondMaturity) const = 0;
    };

    //! Term-structure consistent model class
    /*! This is a base class for models that can reprice exactly
        any discount bond.

        \ingroup shortrate
    */
    class TermStructureConsistentModel : public virtual Observable {
      public:
        TermStructureConsistentModel(
                              const Handle<YieldTermStructure>& termStructure)
        : termStructure_(termStructure) {}
        const Handle<YieldTermStructure>& termStructure() const {
            return termStructure_;
        }
      private:
        Handle<YieldTermStructure> termStructure_;
    };

    //! Calibrated model class
    class CalibratedModel : public Observer, public virtual Observable {
      public:
        CalibratedModel(Size nArguments);

        void update() {
            generateArguments();
            notifyObservers();
        }

        //! Calibrate to a set of market instruments (caps/swaptions)
        /*! An additional constraint can be passed which must be
            satisfied in addition to the constraints of the model.
        */
        void calibrate(
                   const std::vector<boost::shared_ptr<CalibrationHelper> >&,
                   OptimizationMethod& method,
                   const EndCriteria& endCriteria,
                   const Constraint& constraint = Constraint(),
                   const std::vector<Real>& weights = std::vector<Real>());

        const boost::shared_ptr<Constraint>& constraint() const;

        //! Returns array of arguments on which calibration is done
        Disposable<Array> params() const;
        virtual void setParams(const Array& params);
      protected:
        virtual void generateArguments() {}

        std::vector<Parameter> arguments_;
        boost::shared_ptr<Constraint> constraint_;

      private:
        //! Constraint imposed on arguments
        class PrivateConstraint;
        //! Calibration cost function class
        class CalibrationFunction;
        friend class CalibrationFunction;
    };

    //! Abstract short-rate model class
    /*! \ingroup shortrate */
    class ShortRateModel : public CalibratedModel {
      public:
        ShortRateModel(Size nArguments);
        virtual boost::shared_ptr<Lattice> tree(const TimeGrid&) const = 0;
    };

    // inline definitions

    inline const boost::shared_ptr<Constraint>&
    CalibratedModel::constraint() const {
        return constraint_;
    }

    class CalibratedModel::PrivateConstraint : public Constraint {
      private:
        class Impl :  public Constraint::Impl {
          public:
            Impl(const std::vector<Parameter>& arguments)
            : arguments_(arguments) {}
            bool test(const Array& params) const {
                Size k=0;
                for (Size i=0; i<arguments_.size(); i++) {
                    Size size = arguments_[i].size();
                    Array testParams(size);
                    for (Size j=0; j<size; j++, k++)
                        testParams[j] = params[k];
                    if (!arguments_[i].testParams(testParams))
                        return false;
                }
                return true;
            }
          private:
            const std::vector<Parameter>& arguments_;
        };
      public:
        PrivateConstraint(const std::vector<Parameter>& arguments)
        : Constraint(boost::shared_ptr<Constraint::Impl>(
                                   new PrivateConstraint::Impl(arguments))) {}
    };

}

#endif