///////////////////////////////////////////////////////////////////////////////
// Name:        tests/smartrowheightcache/smartrowheightcachetest.cpp
// Purpose:     smart row height cache for dataview unit test
// Author:      Jens Goefpert
// Created:     2018-03-06
// Copyright:   (c) 2018 Jens Goepfert
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "testprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#endif

#include "wx/private/smartrowheightcache.h"

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// test class
// ----------------------------------------------------------------------------

class SmartRowHeightCacheTestCase : public CppUnit::TestCase
{
public:
    SmartRowHeightCacheTestCase() { }

    virtual void setUp();
    virtual void tearDown();

protected:

private:
    CPPUNIT_TEST_SUITE( SmartRowHeightCacheTestCase );
        CPPUNIT_TEST(TestRowRangesSimple);
        CPPUNIT_TEST(TestRowRangesGapsMod2);
        CPPUNIT_TEST(TestHeightCache);
    CPPUNIT_TEST_SUITE_END();

    void TestRowRangesSimple();
    void TestRowRangesGapsMod2();
    void TestHeightCache();

    wxDECLARE_NO_COPY_CLASS(SmartRowHeightCacheTestCase);
};

// register in the unnamed registry so that these tests are run by default
CPPUNIT_TEST_SUITE_REGISTRATION( SmartRowHeightCacheTestCase );

// also include in its own registry so that these tests can be run alone
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( SmartRowHeightCacheTestCase,
                                        "SmartRowHeightCacheTestCase" );

void SmartRowHeightCacheTestCase::setUp()
{
}

void SmartRowHeightCacheTestCase::tearDown()
{
}

// ----------------------------------------------------------------------------
// TestRowRangesAdd
// ----------------------------------------------------------------------------
void SmartRowHeightCacheTestCase::TestRowRangesSimple()
{
    wxLogDebug("TestRowRangesSimple()");

    RowRanges *rr = new RowRanges();

    CPPUNIT_ASSERT_EQUAL(rr->CountAll(), 0);

    for (int i = 0; i <= 10; i++)
    {
        CPPUNIT_ASSERT_EQUAL(rr->Has(i), 0);

        rr->Add(i);

        CPPUNIT_ASSERT_EQUAL(rr->CountAll(), i+1);
        CPPUNIT_ASSERT_EQUAL(rr->CountTo(i), i);
        CPPUNIT_ASSERT_EQUAL(rr->Has(i), 1);
    }

    CPPUNIT_ASSERT_EQUAL(rr->CountAll(), 11);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(10), 10);

    for (int i = 10; i >= 0; i--)
    {
        CPPUNIT_ASSERT_EQUAL(rr->CountAll(), i+1);
        CPPUNIT_ASSERT_EQUAL(rr->CountTo(i), i);

        rr->Remove(i);

        CPPUNIT_ASSERT_EQUAL(rr->CountAll(), i);
        CPPUNIT_ASSERT_EQUAL(rr->CountTo(i), i);
    }

    CPPUNIT_ASSERT_EQUAL(rr->CountAll(), 0);
    for (int i = 10; i >= 0; i--)
    {
        CPPUNIT_ASSERT_EQUAL(rr->CountTo(i), 0);
    }
}

// ----------------------------------------------------------------------------
// TestRowRangesRemove
// ----------------------------------------------------------------------------
void SmartRowHeightCacheTestCase::TestRowRangesGapsMod2()
{
    wxLogDebug("TestRowRangesGapsMod2()");
    RowRanges *rr = new RowRanges();
    for (int i = 0; i < 100; i++)
    {
        CPPUNIT_ASSERT_EQUAL(rr->Has(i), 0);

        if (i % 2 == 0)
        {
            rr->Add(i);
        }
    }
    CPPUNIT_ASSERT_EQUAL(rr->CountAll(), 50);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(100), 50);

    for (int i = 99; i >= 0; i--)
    {
        if (i % 2 == 0)
        {
            CPPUNIT_ASSERT_EQUAL(rr->Has(i), 1);
        }
        else
        {
            CPPUNIT_ASSERT_EQUAL(rr->Has(i), 0);
            rr->Remove(i);
        }
    }

    rr->Remove(1);

    CPPUNIT_ASSERT_EQUAL(rr->CountAll(), 1);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(0), 0);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(1), 1);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(100), 1);

    rr->Remove(0);

    CPPUNIT_ASSERT_EQUAL(rr->CountAll(), 0);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(0), 0);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(1), 0);
    CPPUNIT_ASSERT_EQUAL(rr->CountTo(100), 0);
}

// ----------------------------------------------------------------------------
// TestHeightCache
// ----------------------------------------------------------------------------
void SmartRowHeightCacheTestCase::TestHeightCache()
{
    wxLogDebug("TestHeightCache()");

    HeightCache *hc = new HeightCache();

    for (int i = 0; i <= 10; i++)
    {
        hc->Put(i, 22);
    }
    for (int i = 15; i <= 17; i++)
    {
        hc->Put(i, 22);
    }
    for (int i = 20; i <= 2000; i++)
    {
        hc->Put(i, 22);
    }

    hc->Put(11, 42);
    hc->Put(12, 42);
    hc->Put(18, 42);

    hc->Put(13, 62);
    hc->Put(14, 62);
    hc->Put(19, 62);

    int start = 0;
    int height = 0;

    CPPUNIT_ASSERT_EQUAL(hc->GetLineStart(1000, start), 1);
    CPPUNIT_ASSERT_EQUAL(start, 22180);

    CPPUNIT_ASSERT_EQUAL(hc->GetLineHeight(1000, height), 1);
    CPPUNIT_ASSERT_EQUAL(height, 22);
}
