/////////////////////////////////////////////////////////////////////////////
// Name:        adjustwordspacingfunctor.h
// Author:      Practice Farm
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ADJUSTWORDSPACINGFUNCTOR_H__
#define __VRV_ADJUSTWORDSPACINGFUNCTOR_H__

#include "functor.h"

namespace vrv {

class ControlElement;
class LayerElement;

//----------------------------------------------------------------------------
// AdjustWordSpacingFunctor
//----------------------------------------------------------------------------

/**
 * This class makes room for words over the staff that would otherwise run into the next words over the staff:
 * a tempo mark over each of several narrow bars (multi-bar rests) is stacked in a staircase, where an engraver
 * widens the bars and sets the words in one line. The music between the start of a tempo or a direction above
 * the staff and the start of the next one is widened by what the first lacks, with the wordsMinGap option
 * (in MEI units) left between them. Two headings (tempo marks, or words set bold as one) are given whatever
 * they lack; other words only a nudge of up to four staff spaces, because an expression under a tempo mark is
 * a stack an engraver would leave. Nothing happens when the option is 0, or for words that start together.
 */
class AdjustWordSpacingFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    AdjustWordSpacingFunctor(Doc *doc);
    virtual ~AdjustWordSpacingFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return true; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitDir(Dir *dir) override;
    FunctorCode VisitTempo(Tempo *tempo) override;
    FunctorCode VisitMeasure(Measure *measure) override;
    FunctorCode VisitMeasureEnd(Measure *measure) override;
    FunctorCode VisitSystem(System *system) override;
    FunctorCode VisitSystemEnd(System *system) override;
    ///@}

protected:
    //
private:
    // Words over the staff, where they start and how far they reach either side of it
    struct Word {
        Measure *measure;
        Alignment *start;
        int x; // the start, counted along the system
        int left; // the reach left of the start (negative or zero)
        int right; // the reach right of the start
        bool heading; // a tempo mark, or words set bold as one
    };
    void Add(ControlElement *element, LayerElement *start);

public:
    //
private:
    // The gap in drawing units, 0 if off
    int m_gap;
    // The words of the current system, in the order met
    std::vector<Word> m_words;
    // The current system, measure and the width of the measures before it
    System *m_currentSystem;
    Measure *m_currentMeasure;
    int m_offset;
};

} // namespace vrv

#endif // __VRV_ADJUSTWORDSPACINGFUNCTOR_H__
