/////////////////////////////////////////////////////////////////////////////
// Name:        adjusthairpinspacingfunctor.cpp
// Author:      Practice Farm
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "adjusthairpinspacingfunctor.h"

//----------------------------------------------------------------------------

#include "doc.h"
#include "dynam.h"
#include "hairpin.h"
#include "measure.h"
#include "system.h"
#include "vrv.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// AdjustHairpinSpacingFunctor
//----------------------------------------------------------------------------

AdjustHairpinSpacingFunctor::AdjustHairpinSpacingFunctor(Doc *doc) : DocFunctor(doc)
{
    m_minLength = m_doc->GetOptions()->m_hairpinMinLength.GetValue() * m_doc->GetDrawingUnit(100);
    m_currentSystem = NULL;
}

FloatingPositioner *AdjustHairpinSpacingFunctor::GetPositioner(FloatingObject *object) const
{
    if (!m_currentSystem) return NULL;
    ArrayOfFloatingPositioners positioners;
    m_currentSystem->m_systemAligner.FindAllPositionerPointingTo(&positioners, object);
    FloatingPositioner *widest = NULL;
    for (FloatingPositioner *positioner : positioners) {
        if (!positioner->HasContentBB()) continue;
        if (!widest || (positioner->GetContentX2() - positioner->GetContentX1())
                > (widest->GetContentX2() - widest->GetContentX1())) {
            widest = positioner;
        }
    }
    return widest;
}

FunctorCode AdjustHairpinSpacingFunctor::VisitHairpin(Hairpin *hairpin)
{
    if (m_minLength <= 0) return FUNCTOR_SIBLINGS;

    LayerElement *start = hairpin->GetStart();
    LayerElement *end = hairpin->GetEnd();
    if (!start || !end || (start == end)) return FUNCTOR_SIBLINGS;
    Alignment *startAlignment = start->GetAlignment();
    Alignment *endAlignment = end->GetAlignment();
    if (!startAlignment || !endAlignment) return FUNCTOR_SIBLINGS;

    Measure *startMeasure = vrv_cast<Measure *>(start->GetFirstAncestor(MEASURE));
    Measure *endMeasure = vrv_cast<Measure *>(end->GetFirstAncestor(MEASURE));
    if (!startMeasure || !endMeasure) return FUNCTOR_SIBLINGS;
    // Only a hairpin that starts in the measure being visited, where its adjustment is applied
    if (startMeasure != vrv_cast<Measure *>(hairpin->GetFirstAncestor(MEASURE))) return FUNCTOR_SIBLINGS;

    const int unit = m_doc->GetDrawingUnit(100);

    // The measures are not aligned yet: x is read within each measure, and the measures in between are added
    int startX = startAlignment->GetXRel();
    int endX = endAlignment->GetXRel();
    if (startMeasure != endMeasure) {
        Object *parent = startMeasure->GetParent();
        if (!parent) return FUNCTOR_SIBLINGS;
        int offset = 0;
        bool found = false;
        int count = 0;
        for (Object *object = startMeasure; object && (count < 8); object = parent->GetNext(object, MEASURE)) {
            if (object == endMeasure) {
                found = true;
                break;
            }
            offset += vrv_cast<Measure *>(object)->GetWidth();
            ++count;
        }
        // Too far apart to be short, or not followed through
        if (!found) return FUNCTOR_SIBLINGS;
        endX += offset;
    }
    else if (endX <= startX) {
        return FUNCTOR_SIBLINGS;
    }

    // What the drawing takes out: the room for a dynamic at either end, read against the note it hangs on
    ControlElement *leftLink = hairpin->GetLeftLink();
    if (leftLink && leftLink->Is(DYNAM)) {
        FloatingPositioner *positioner = this->GetPositioner(leftLink);
        if (positioner) startX += std::max(0, positioner->GetContentRight() + unit / 2 - start->GetDrawingX());
    }
    ControlElement *rightLink = hairpin->GetRightLink();
    if (rightLink && rightLink->Is(DYNAM)) {
        FloatingPositioner *positioner = this->GetPositioner(rightLink);
        if (positioner) endX -= std::max(0, end->GetDrawingX() - (positioner->GetContentLeft() - unit / 2));
    }
    // And the room it keeps from a barline it would touch
    if (startMeasure == endMeasure) {
        const int rightBarLineX = startMeasure->GetRightBarLine()->GetAlignment()->GetXRel();
        endX = std::min(endX, rightBarLineX - 2 * unit);
    }

    const int shortfall = m_minLength - (endX - startX);
    if (shortfall <= 0) return FUNCTOR_SIBLINGS;

    // Within one measure the notes under the hairpin are spread; over a barline, the rest of the first measure
    Alignment *until = (startMeasure == endMeasure) ? endAlignment : startMeasure->GetRightBarLine()->GetAlignment();
    if (!until || (until->GetXRel() <= startAlignment->GetXRel())) return FUNCTOR_SIBLINGS;
    m_adjustments.push_back(std::make_tuple(startAlignment, until, shortfall));

    return FUNCTOR_SIBLINGS;
}

FunctorCode AdjustHairpinSpacingFunctor::VisitMeasureEnd(Measure *measure)
{
    if (!m_adjustments.empty()) {
        measure->m_measureAligner.AdjustProportionally(m_adjustments);
        m_adjustments.clear();
    }
    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustHairpinSpacingFunctor::VisitSystem(System *system)
{
    m_currentSystem = system;
    m_adjustments.clear();
    return FUNCTOR_CONTINUE;
}

} // namespace vrv
