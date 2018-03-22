///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/smartrowheightcache.cpp
// Purpose:     Smart height cache of rows in a dataview
// Author:      Jens Goepfert
// Created:     2018-03-06
// Copyright:   (c) 2018 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/private/smartrowheightcache.h"

// ----------------------------------------------------------------------------
// private structs
// ----------------------------------------------------------------------------

// ============================================================================
// implementation
// ============================================================================


// ----------------------------------------------------------------------------
// RowRanges
// ----------------------------------------------------------------------------

void RowRanges::Add(const unsigned int row)
{
    size_t count = m_ranges.size();
    size_t rngIdx = 0;
    for (rngIdx = 0; rngIdx < count; ++rngIdx)
    {
        RowRange &rng = m_ranges[rngIdx];

        if (row >= rng.from && rng.to > row)
        {
            // index already in range
            return;
        }

        if (row == rng.from - 1)
        {
            // extend range at the beginning (to the left)
            rng.from = row;
            // no cleanup necessary
            return;
        }
        if (row == rng.to)
        {
            // extend range at the end (set to row+1 because 'to' is not including)
            rng.to = row + 1;
            CleanUp(rngIdx);
            return;
        }

        if (rng.from > row + 1)
        {
            // this range is already behind row index, so break here and insert a new range before
            break;
        }
    }
    //    wxLogMessage("New Range: %d" , count);

    RowRange newRange;
    newRange.from = row;
    newRange.to = row + 1;
    m_ranges.insert(m_ranges.begin() + rngIdx, newRange);
}

void RowRanges::Remove(const unsigned int row)
{
    size_t count = m_ranges.size();
    size_t rngIdx = 0;
    while (rngIdx < count)
    {
        RowRange &rng = m_ranges[rngIdx];
        if (rng.from >= row)
        {
            // this range starts behind row index, so remove it
            m_ranges.erase(m_ranges.begin() + rngIdx);
            count--;
            continue;
        }
        if (rng.to > row)
        {
            // this ranges includes row, so cut off at row index to exclude row
            rng.to = row;
        }

        rngIdx += 1;
    }
}


void RowRanges::CleanUp(int idx)
{
    size_t count = m_ranges.size();
    size_t rngIdx = 0;
    if (idx > 0)
    {
        // start one RowRange before
        rngIdx = idx - 1;
    }
    if (idx >= count)
    {
        // should never reached, due CleanUp is private and internal called correctly
        return;
    }
    RowRange *prevRng = &m_ranges[rngIdx];
    rngIdx++;
    while (rngIdx <= idx + 1 && rngIdx < count)
    {
        RowRange &rng = m_ranges[rngIdx];

        if (prevRng->to == rng.from)
        {
            // this range starts where the previous range began, so remove this
            // and set the to-value of the previous range to the to-value of this range
            prevRng->to = rng.to;
            m_ranges.erase(m_ranges.begin() + rngIdx);
            count--;
            continue;
        }

        prevRng = &rng;
        rngIdx += 1;
    }
}

bool RowRanges::Has(unsigned int row) const
{
    size_t count = m_ranges.size();
    for (size_t rngIdx = 0; rngIdx < count; rngIdx++)
    {
        const RowRange &rng = m_ranges[rngIdx];
        if (rng.from <= row && row < rng.to)
        {
            return true;
        }
    }
    return false;
}

unsigned int RowRanges::CountAll() const
{
    unsigned int ctr = 0;
    size_t count = m_ranges.size();
    for (size_t rngIdx = 0; rngIdx < count; rngIdx++)
    {
        const RowRange &rng = m_ranges[rngIdx];
        ctr += rng.to - rng.from;
    }
    return ctr;
}

unsigned int RowRanges::CountTo(unsigned int row) const
{
    unsigned int ctr = 0;
    size_t count = m_ranges.size();
    for (size_t rngIdx = 0; rngIdx < count; rngIdx++)
    {
        const RowRange &rng = m_ranges[rngIdx];
        if (rng.from > row)
        {
            break;
        }
        else if (rng.to < row)
        {
            ctr += rng.to - rng.from;
        }
        else
        {
            ctr += row - rng.from;
            break;
        }
    }
    return ctr;
}

unsigned int RowRanges::GetSize() const // for debugging statistics
{
    return m_ranges.size();
}


// ----------------------------------------------------------------------------
// HeightCache
// ----------------------------------------------------------------------------

bool HeightCache::GetLineInfo(unsigned int row, int &start, int &height)
{
    int y = 0;
    bool found = false;
    HeightToRowRangesMap::iterator it;
    for (it = m_heightToRowRange.begin(); it != m_heightToRowRange.end(); ++it)
    {
        int rowHeight = it->first;
        RowRanges* rowRanges = it->second;
        if (rowRanges->Has(row))
        {
            height = rowHeight;
            found = true;
        }
        y += rowHeight * (rowRanges->CountTo(row));
    }
    if (found)
    {
        start = y;
    }
    return found;
}

bool HeightCache::GetLineStart(unsigned int row, int &start)
{
    int height = 0;
    return GetLineInfo(row, start, height);
}

bool HeightCache::GetLineHeight(unsigned int row, int &height)
{
    HeightToRowRangesMap::iterator it;
    for (it = m_heightToRowRange.begin(); it != m_heightToRowRange.end(); ++it)
    {
        int rowHeight = it->first;
        RowRanges* rowRanges = it->second;
        if (rowRanges->Has(row))
        {
            height = rowHeight;
            return true;
        }
    }
    return false;
}

bool HeightCache::GetLineAt(int y, unsigned int &row)
{
    unsigned int total = 0;
    HeightToRowRangesMap::iterator it;
    for (it = m_heightToRowRange.begin(); it != m_heightToRowRange.end(); ++it)
    {
        RowRanges* rowRanges = it->second;
        total += rowRanges->CountAll();
    }

    if (total == 0)
    {
        return false;
    }

    int lo = 0;
    int hi = total;
    int start, height;
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        if (GetLineInfo(mid, start, height))
        {
            if (start + height <= y)
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }
        else
        {
            // should never happen, except the HeightCache has gaps which is an invalid state
            return false;
        }
    }
    if (GetLineInfo(lo, start, height))
    {
        if (y < start)
        {
            // given y point is before the first row
            return false;
        }
        row = lo;
        return true;
    }
    else
    {
        // given y point is after the last row
        return false;
    }
}

void HeightCache::Put(const unsigned int row, const int height)
{
    RowRanges *rowRanges = m_heightToRowRange[height];
    if (rowRanges == NULL)
    {
        rowRanges = new RowRanges();
        m_heightToRowRange[height] = rowRanges;
    }
    rowRanges->Add(row);
    m_showLogInfo = true;
}

void HeightCache::Remove(const unsigned int row)
{
    HeightToRowRangesMap::iterator it;
    for (it = m_heightToRowRange.begin(); it != m_heightToRowRange.end(); ++it)
    {
        RowRanges* rowRanges = it->second;
        rowRanges->Remove(row);
    }
}

void HeightCache::Clear()
{
    HeightToRowRangesMap::iterator it;
    for (it = m_heightToRowRange.begin(); it != m_heightToRowRange.end(); ++it)
    {
        RowRanges* rowRanges = it->second;
        delete rowRanges;
    }
    m_heightToRowRange.clear();
    m_showLogInfo = true;
}

void HeightCache::LogSize() // for debugging statistics
{
    if (!m_showLogInfo)
    {
        return;
    }

    int rowRangeCount = 0;
    HeightToRowRangesMap::iterator it;
    for (it = m_heightToRowRange.begin(); it != m_heightToRowRange.end(); ++it)
    {
        RowRanges* rowRanges = it->second;
        rowRangeCount += rowRanges->GetSize();
    }
    int heights = m_heightToRowRange.size();

    wxLogMessage("Cache size: HeightMap=%d; RowRanges=%d --> %d", heights, rowRangeCount, sizeof(RowRange) * rowRangeCount);
    m_showLogInfo = false;
}
