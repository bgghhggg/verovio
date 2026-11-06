/////////////////////////////////////////////////////////////////////////////
// Name:        adjusthairpinspacingfunctor.h
// Author:      Practice Farm
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ADJUSTHAIRPINSPACINGFUNCTOR_H__
#define __VRV_ADJUSTHAIRPINSPACINGFUNCTOR_H__

#include "functor.h"

namespace vrv {

//----------------------------------------------------------------------------
// AdjustHairpinSpacingFunctor
//----------------------------------------------------------------------------

/**
 * This class widens the music under a hairpin that would otherwise be drawn shorter than the hairpinMinLength
 * option, as an engraver makes room for a hairpin rather than squeezing it. The room left for a dynamic the
 * hairpin starts or ends on is counted out of its length, as the drawing does. Nothing happens when the option is 0.
 */
class AdjustHairpinSpacingFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    AdjustHairpinSpacingFunctor(Doc *doc);
    virtual ~AdjustHairpinSpacingFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return true; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitHairpin(Hairpin *hairpin) override;
    FunctorCode VisitMeasureEnd(Measure *measure) override;
    FunctorCode VisitSystem(System *system) override;
    ///@}

protected:
    //
private:
    // The widest positioner of an object in the current system, if it has a content box
    FloatingPositioner *GetPositioner(FloatingObject *object) const;

public:
    //
private:
    // The minimum length in drawing units, 0 if off
    int m_minLength;
    // The adjustment tuples (Alignment start|Alignment end|distance) of the current measure
    ArrayOfAdjustmentTuples m_adjustments;
    // The current system
    System *m_currentSystem;
};

} // namespace vrv

#endif // __VRV_ADJUSTHAIRPINSPACINGFUNCTOR_H__
