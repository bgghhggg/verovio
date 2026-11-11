/////////////////////////////////////////////////////////////////////////////
// Name:        adjustwordspacingfunctor.cpp
// Author:      Practice Farm
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "adjustwordspacingfunctor.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <map>

#include "dir.h"
#include "doc.h"
#include "measure.h"
#include "rend.h"
#include "system.h"
#include "tempo.h"
#include "vrv.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// AdjustWordSpacingFunctor
//----------------------------------------------------------------------------

AdjustWordSpacingFunctor::AdjustWordSpacingFunctor(Doc *doc) : DocFunctor(doc)
{
    m_gap = m_doc->GetOptions()->m_wordsMinGap.GetValue() * m_doc->GetDrawingUnit(100);
    m_currentSystem = NULL;
    m_currentMeasure = NULL;
    m_offset = 0;
}

void AdjustWordSpacingFunctor::Add(ControlElement *element, LayerElement *start)
{
    if ((m_gap <= 0) || !m_currentSystem || !m_currentMeasure || !start) return;
    Alignment *alignment = start->GetAlignment();
    if (!alignment) return;
    // Only words that start in the measure being visited
    if (vrv_cast<Measure *>(start->GetFirstAncestor(MEASURE)) != m_currentMeasure) return;

    ArrayOfFloatingPositioners positioners;
    m_currentSystem->m_systemAligner.FindAllPositionerPointingTo(&positioners, element);
    FloatingPositioner *widest = NULL;
    for (FloatingPositioner *positioner : positioners) {
        if (!positioner->HasContentBB()) continue;
        if (!widest
            || (positioner->GetContentX2() - positioner->GetContentX1())
                > (widest->GetContentX2() - widest->GetContentX1())) {
            widest = positioner;
        }
    }
    if (!widest) return;

    const int startX = start->GetDrawingX();
    Word word;
    word.measure = m_currentMeasure;
    word.start = alignment;
    word.x = m_offset + alignment->GetXRel();
    word.left = std::min(0, widest->GetContentLeft() - startX);
    word.right = std::max(0, widest->GetContentRight() - startX);
    word.heading = element->Is(TEMPO);
    if (!word.heading) {
        ListOfObjects rends = element->FindAllDescendantsByType(REND);
        for (Object *object : rends) {
            if (vrv_cast<Rend *>(object)->GetFontweight() == FONTWEIGHT_bold) word.heading = true;
        }
    }
    m_words.push_back(word);
}

FunctorCode AdjustWordSpacingFunctor::VisitDir(Dir *dir)
{
    if (dir->GetPlace() == STAFFREL_above) this->Add(dir, dir->GetStart());
    return FUNCTOR_SIBLINGS;
}

FunctorCode AdjustWordSpacingFunctor::VisitTempo(Tempo *tempo)
{
    if (tempo->GetPlace() != STAFFREL_below) this->Add(tempo, tempo->GetStart());
    return FUNCTOR_SIBLINGS;
}

FunctorCode AdjustWordSpacingFunctor::VisitMeasure(Measure *measure)
{
    m_currentMeasure = measure;
    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustWordSpacingFunctor::VisitMeasureEnd(Measure *measure)
{
    m_offset += measure->GetWidth();
    m_currentMeasure = NULL;
    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustWordSpacingFunctor::VisitSystem(System *system)
{
    m_currentSystem = system;
    m_words.clear();
    m_offset = 0;
    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustWordSpacingFunctor::VisitSystemEnd(System *system)
{
    if ((m_gap > 0) && (m_words.size() > 1)) {
        const int unit = m_doc->GetDrawingUnit(100);
        std::stable_sort(m_words.begin(), m_words.end(), [](const Word &a, const Word &b) { return a.x < b.x; });
        std::map<Measure *, ArrayOfAdjustmentTuples> adjustments;
        for (size_t i = 0; i + 1 < m_words.size(); ++i) {
            const Word &word = m_words[i];
            const Word &next = m_words[i + 1];
            // Words that start together are a stack, and stay one
            if (next.x - word.x < unit) continue;
            const int shortfall = (word.x + word.right + m_gap) - (next.x + next.left);
            // Nothing lacking; or so much that the bar would be mostly air: the stack is the lesser evil
            const int most = (word.heading && next.heading) ? 40 * unit : 8 * unit;
            if ((shortfall <= 0) || (shortfall > most)) continue;
            Alignment *until = (word.measure == next.measure)
                ? next.start
                : word.measure->GetRightBarLine()->GetAlignment();
            if (!until || (until->GetXRel() <= word.start->GetXRel())) continue;
            adjustments[word.measure].push_back(std::make_tuple(word.start, until, shortfall));
        }
        for (auto &entry : adjustments) {
            entry.first->m_measureAligner.AdjustProportionally(entry.second);
        }
    }
    m_words.clear();
    m_currentSystem = NULL;
    return FUNCTOR_CONTINUE;
}

} // namespace vrv
