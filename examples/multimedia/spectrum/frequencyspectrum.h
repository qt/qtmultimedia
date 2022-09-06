// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FREQUENCYSPECTRUM_H
#define FREQUENCYSPECTRUM_H

#include <QList>

/**
 * Represents a frequency spectrum as a series of elements, each of which
 * consists of a frequency, an amplitude and a phase.
 */
class FrequencySpectrum
{
public:
    FrequencySpectrum(int numPoints = 0);

    struct Element
    {
        Element() : frequency(0.0), amplitude(0.0), phase(0.0), clipped(false) { }

        /**
         * Frequency in Hertz
         */
        qreal frequency;

        /**
         * Amplitude in range [0.0, 1.0]
         */
        qreal amplitude;

        /**
         * Phase in range [0.0, 2*PI]
         */
        qreal phase;

        /**
         * Indicates whether value has been clipped during spectrum analysis
         */
        bool clipped;
    };

    typedef QList<Element>::iterator iterator;
    typedef QList<Element>::const_iterator const_iterator;

    void reset();

    int count() const;
    Element &operator[](int index);
    const Element &operator[](int index) const;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    QList<Element> m_elements;
};

#endif // FREQUENCYSPECTRUM_H
