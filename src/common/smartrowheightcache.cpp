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

#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(ArrayOfRowRange)


// ----------------------------------------------------------------------------
// RowRanges
// ----------------------------------------------------------------------------

void RowRanges::Add(const unsigned int idx)
{
    size_t count = m_ranges.GetCount();
    size_t rngIdx = 0;
    for (rngIdx = 0; rngIdx < count; ++rngIdx)
    {
        RowRange& rng = m_ranges[rngIdx];

        if (idx >= rng.from && rng.to >= idx)
        {
            // index already in range
            return;
        }

        if (rng.from == idx + 1)
        {
            rng.from = idx;
            CleanUp(rngIdx);
            return;
        }
        if (rng.to == idx - 1)
        {
            rng.to = idx;
            CleanUp(rngIdx);
            return;
        }

        if (rng.from > idx + 1)
        {
            break;
        }
    }
    //    wxLogMessage("New Range: %d" , count);

    RowRange newRange;
    newRange.from = idx;
    newRange.to = idx;
    m_ranges.Insert(newRange, rngIdx);
}

void RowRanges::Remove(const unsigned int idx)
{
    size_t count = m_ranges.GetCount();
    size_t rngIdx = 0;
    while (rngIdx < count)
    {
        RowRange& rng = m_ranges[rngIdx];
        if (rng.from >= idx)
        {
            m_ranges.RemoveAt(rngIdx);
            count--;
            continue;
        }
        if (rng.to >= idx)
        {
            rng.to = idx - 1;
        }

        rngIdx += 1;
    }
}


void RowRanges::CleanUp(int idx)
{
    RowRange *prevRng = NULL;

    size_t count = m_ranges.GetCount();
    size_t rngIdx = 0;
    if (idx > 0)
    {
        rngIdx = idx - 1;
    }
    while (rngIdx <= idx + 1 && rngIdx < count)
    {
        RowRange& rng = m_ranges[rngIdx];

        if (prevRng != NULL && prevRng->to >= rng.to)
        {
            m_ranges.RemoveAt(rngIdx);
            count--;
            continue;
        }

        prevRng = &rng;
        rngIdx += 1;
    }

}

bool RowRanges::Has(unsigned int idx) const
{
    size_t count = m_ranges.GetCount();
    for (size_t rngIdx = 0; rngIdx < count; rngIdx++)
    {
        const RowRange& rng = m_ranges[rngIdx];
        if (rng.from <= idx && idx <= rng.to)
        {
            return true;
        }
    }
    return false;
}

unsigned int RowRanges::CountAll() const
{
    unsigned int ctr = 0;
    size_t count = m_ranges.GetCount();
    for (size_t rngIdx = 0; rngIdx < count; rngIdx++)
    {
        const RowRange& rng = m_ranges[rngIdx];
        ctr += (rng.to + 1) - rng.from;
    }
    return ctr;
}

unsigned int RowRanges::CountTo(unsigned int idx) const
{
    unsigned int ctr = 0;
    size_t count = m_ranges.GetCount();
    for (size_t rngIdx = 0; rngIdx < count; rngIdx++)
    {
        const RowRange& rng = m_ranges[rngIdx];
        if (rng.from > idx)
        {
            break;
        }
        else if (rng.to < idx)
        {
            ctr += rng.to - rng.from;
            ctr += 1;
        }
        else
        {
            ctr += idx - rng.from;
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
    while (lo < hi)
    {
        int mid = (lo + hi) / 2;
        int start, height;
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
            return false;
        }
    }
    row = lo;
    return true;
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


