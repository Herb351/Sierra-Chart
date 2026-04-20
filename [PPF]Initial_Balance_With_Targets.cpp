#include "sierrachart.h"
#include <cmath>

// Helper used by the target logic (was missing before)
static int Get1Up(double x)
{
    if (x <= 0.0)
        return 0;
    return (int)std::floor(x);
}

SCDLLName("[PPF]Initial Balance With Targets")

SCSFExport scsf_InitialBalance_With_Targets(SCStudyInterfaceRef sc)
{
    SCSubgraphRef Subgraph_IBHExt6  = sc.Subgraph[0];
    SCSubgraphRef Subgraph_IBHExt5  = sc.Subgraph[1];
    SCSubgraphRef Subgraph_IBHExt4  = sc.Subgraph[2];
    SCSubgraphRef Subgraph_IBHExt3  = sc.Subgraph[3];
    SCSubgraphRef Subgraph_IBHExt2  = sc.Subgraph[4];
    SCSubgraphRef Subgraph_IBHExt1  = sc.Subgraph[5];
    SCSubgraphRef Subgraph_IBHigh   = sc.Subgraph[6];
    SCSubgraphRef Subgraph_IBMid    = sc.Subgraph[7];
    SCSubgraphRef Subgraph_IBLow    = sc.Subgraph[8];
    SCSubgraphRef Subgraph_IBLExt1  = sc.Subgraph[9];
    SCSubgraphRef Subgraph_IBLExt2  = sc.Subgraph[10];
    SCSubgraphRef Subgraph_IBLExt3  = sc.Subgraph[11];
    SCSubgraphRef Subgraph_IBLExt4  = sc.Subgraph[12];
    SCSubgraphRef Subgraph_IBLExt5  = sc.Subgraph[13];
    SCSubgraphRef Subgraph_IBLExt6  = sc.Subgraph[14];
    SCSubgraphRef Subgraph_IBRegionTop      = sc.Subgraph[15];
    SCSubgraphRef Subgraph_IBRegionBottom   = sc.Subgraph[16];

    // Targets (up to 6 above and below)
    SCSubgraphRef SG_TUp1 = sc.Subgraph[17];
    SCSubgraphRef SG_TUp2 = sc.Subgraph[18];
    SCSubgraphRef SG_TUp3 = sc.Subgraph[19];
    SCSubgraphRef SG_TUp4 = sc.Subgraph[20];
    SCSubgraphRef SG_TUp5 = sc.Subgraph[21];
    SCSubgraphRef SG_TUp6 = sc.Subgraph[22];

    SCSubgraphRef SG_TDn1 = sc.Subgraph[23];
    SCSubgraphRef SG_TDn2 = sc.Subgraph[24];
    SCSubgraphRef SG_TDn3 = sc.Subgraph[25];
    SCSubgraphRef SG_TDn4 = sc.Subgraph[26];
    SCSubgraphRef SG_TDn5 = sc.Subgraph[27];
    SCSubgraphRef SG_TDn6 = sc.Subgraph[28];

    // Breakout signals
    SCSubgraphRef SG_SignalUp = sc.Subgraph[29];
    SCSubgraphRef SG_SignalDn = sc.Subgraph[30];

    SCInputRef Input_IBType      = sc.Input[0];
    SCInputRef Input_StartTime   = sc.Input[1];
    SCInputRef Input_EndTime     = sc.Input[2];
    SCInputRef Input_NumDays     = sc.Input[3];
    SCInputRef Input_RoundExt    = sc.Input[4];
    SCInputRef Input_NumberDaysToCalculate = sc.Input[5];
    SCInputRef Input_NumberOfMinutes      = sc.Input[6];
    SCInputRef Input_StartEndTimeMethod = sc.Input[7];
    SCInputRef Input_PeriodEndAsMinutesFromSessionStart = sc.Input[8];

    SCInputRef Input_Multiplier1 = sc.Input[10];
    SCInputRef Input_Multiplier2 = sc.Input[11];
    SCInputRef Input_Multiplier3 = sc.Input[12];
    SCInputRef Input_Multiplier4 = sc.Input[13];
    SCInputRef Input_Multiplier5 = sc.Input[14];
    SCInputRef Input_Multiplier6 = sc.Input[15];

    // NEW inputs
    SCInputRef Input_ShowAdaptiveTargets = sc.Input[16];
    SCInputRef Input_TargetPercentOfIBRange = sc.Input[17]; // percent (e.g. 50)
    SCInputRef Input_TargetDisplayMode = sc.Input[18];      // 0 adaptive, 1 extended
    SCInputRef Input_TargetSource = sc.Input[19];           // 0 close, 1 highs/lows
    SCInputRef Input_ShowSignals = sc.Input[20];
    SCInputRef Input_SignalBiasMode = sc.Input[21];         // 0 none, 1 midline bias
    SCInputRef Input_Debug = sc.Input[22];

    // NEW: configurable signal offset
    SCInputRef Input_SignalOffsetTicks = sc.Input[23];

    if (sc.SetDefaults)
    {
        sc.GraphName        = "Initial Balance";
        sc.DrawZeros        = 0;
        sc.GraphRegion      = 0;
        sc.AutoLoop         = 1;

        sc.ScaleRangeType = SCALE_SAMEASREGION;
        sc.DrawStudyUnderneathMainPriceGraph = 1;

        Subgraph_IBHExt6.Name = "IB High Ext 3";
        Subgraph_IBHExt6.PrimaryColor = RGB(0, 255, 0);
        Subgraph_IBHExt6.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBHExt6.DrawZeros = false;

        Subgraph_IBHExt5.Name = "IB High 50%";
        Subgraph_IBHExt5.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBHExt5.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBHExt5.LineStyle = LINESTYLE_DASH;
        Subgraph_IBHExt5.DrawZeros = false;

        Subgraph_IBHExt4.Name = "IB High Ext 2";
        Subgraph_IBHExt4.PrimaryColor = RGB(0, 255, 0);
        Subgraph_IBHExt4.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBHExt4.DrawZeros = false;

        Subgraph_IBHExt3.Name = "IB High 50%";
        Subgraph_IBHExt3.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBHExt3.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBHExt3.LineStyle = LINESTYLE_DASH;
        Subgraph_IBHExt3.DrawZeros = false;

        Subgraph_IBHExt2.Name = "IB High Ext 1";
        Subgraph_IBHExt2.PrimaryColor = RGB(0, 255, 0);
        Subgraph_IBHExt2.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBHExt2.DrawZeros = false;

        Subgraph_IBHExt1.Name = "IB High 50%";
        Subgraph_IBHExt1.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBHExt1.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBHExt1.LineStyle = LINESTYLE_DASH;
        Subgraph_IBHExt1.DrawZeros = false;

        Subgraph_IBHigh.Name = "IB High";
        Subgraph_IBHigh.PrimaryColor = RGB(0, 255, 0);
        Subgraph_IBHigh.DrawStyle = DRAWSTYLE_DASH;
        Subgraph_IBHigh.DrawZeros = false;

        Subgraph_IBMid.Name = "IB 50%";
        Subgraph_IBMid.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBMid.DrawStyle = DRAWSTYLE_DASH;
        Subgraph_IBMid.LineStyle = LINESTYLE_DASH;
        Subgraph_IBMid.DrawZeros = false;

        Subgraph_IBLow.Name = "IB Low";
        Subgraph_IBLow.PrimaryColor = RGB(255, 0, 0);
        Subgraph_IBLow.DrawStyle = DRAWSTYLE_DASH;
        Subgraph_IBLow.DrawZeros = false;

        Subgraph_IBLExt1.Name = "IB Low 50%";
        Subgraph_IBLExt1.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBLExt1.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBLExt1.LineStyle = LINESTYLE_DASH;
        Subgraph_IBLExt1.DrawZeros = false;

        Subgraph_IBLExt2.Name = "IB Low Ext 1";
        Subgraph_IBLExt2.PrimaryColor = RGB(255, 0, 0);
        Subgraph_IBLExt2.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBLExt2.DrawZeros = false;

        Subgraph_IBLExt3.Name = "IB Low 50%";
        Subgraph_IBLExt3.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBLExt3.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBLExt3.LineStyle = LINESTYLE_DASH;
        Subgraph_IBLExt3.DrawZeros = false;

        Subgraph_IBLExt4.Name = "IB Low Ext 2";
        Subgraph_IBLExt4.PrimaryColor = RGB(255, 0, 0);
        Subgraph_IBLExt4.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBLExt4.DrawZeros = false;

        Subgraph_IBLExt5.Name = "IB Low 50%";
        Subgraph_IBLExt5.PrimaryColor = RGB(255, 128, 0);
        Subgraph_IBLExt5.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBLExt5.LineStyle = LINESTYLE_DASH;
        Subgraph_IBLExt5.DrawZeros = false;

        Subgraph_IBLExt6.Name = "IB Low Ext 3";
        Subgraph_IBLExt6.PrimaryColor = RGB(255, 0, 0);
        Subgraph_IBLExt6.DrawStyle = DRAWSTYLE_IGNORE;
        Subgraph_IBLExt6.DrawZeros = false;

        Subgraph_IBRegionTop.Name = "IB Region Top";
        Subgraph_IBRegionTop.PrimaryColor = COLOR_YELLOW;
        Subgraph_IBRegionTop.DrawStyle = DRAWSTYLE_TRANSPARENT_FILL_RECTANGLE_TOP;
        Subgraph_IBRegionTop.DrawZeros = false;

        Subgraph_IBRegionBottom.Name = "IB Region Bottom";
        Subgraph_IBRegionBottom.PrimaryColor = COLOR_YELLOW;
        Subgraph_IBRegionBottom.DrawStyle = DRAWSTYLE_TRANSPARENT_FILL_RECTANGLE_BOTTOM;
        Subgraph_IBRegionBottom.DrawZeros = false;

        // Targets defaults
        auto InitTargetSG = [&](SCSubgraphRef sg, const char* name, COLORREF c)
        {
            sg.Name = name;
            sg.PrimaryColor = c;
            sg.DrawStyle = DRAWSTYLE_LINE;
            sg.LineStyle = LINESTYLE_DOT;
            sg.LineWidth = 1;
            sg.DrawZeros = false;
        };

        InitTargetSG(SG_TUp1, "Target Up 1", RGB(8, 153, 129));
        InitTargetSG(SG_TUp2, "Target Up 2", RGB(8, 153, 129));
        InitTargetSG(SG_TUp3, "Target Up 3", RGB(8, 153, 129));
        InitTargetSG(SG_TUp4, "Target Up 4", RGB(8, 153, 129));
        InitTargetSG(SG_TUp5, "Target Up 5", RGB(8, 153, 129));
        InitTargetSG(SG_TUp6, "Target Up 6", RGB(8, 153, 129));

        InitTargetSG(SG_TDn1, "Target Down 1", RGB(242, 54, 69));
        InitTargetSG(SG_TDn2, "Target Down 2", RGB(242, 54, 69));
        InitTargetSG(SG_TDn3, "Target Down 3", RGB(242, 54, 69));
        InitTargetSG(SG_TDn4, "Target Down 4", RGB(242, 54, 69));
        InitTargetSG(SG_TDn5, "Target Down 5", RGB(242, 54, 69));
        InitTargetSG(SG_TDn6, "Target Down 6", RGB(242, 54, 69));

        // Signal styles (compatible: no DRAWSTYLE_MARKER / MarkerStyle)
        SG_SignalUp.Name = "Bullish Signal";
        SG_SignalUp.DrawStyle = DRAWSTYLE_ARROW_UP;
        SG_SignalUp.PrimaryColor = RGB(8, 153, 129);
        SG_SignalUp.LineWidth = 2;
        SG_SignalUp.DrawZeros = false;

        SG_SignalDn.Name = "Bearish Signal";
        SG_SignalDn.DrawStyle = DRAWSTYLE_ARROW_DOWN;
        SG_SignalDn.PrimaryColor = RGB(242, 54, 69);
        SG_SignalDn.LineWidth = 2;
        SG_SignalDn.DrawZeros = false;

        // Inputs (existing)
        Input_IBType.Name = "Initial Balance Type";
        Input_IBType.SetCustomInputStrings("Daily;Weekly;Weekly Include Sunday;Intraday");
        Input_IBType.SetCustomInputIndex(0);

        Input_StartTime.Name = "Start Time";
        Input_StartTime.SetTime(HMS_TIME(9, 30, 0));

        Input_EndTime.Name = "End Time";
        Input_EndTime.SetTime(HMS_TIME(10, 29, 59));

        Input_NumDays.Name = "Weekly: Number of Days";
        Input_NumDays.SetInt(2);
        Input_NumDays.SetIntLimits(1, 5);

        Input_NumberOfMinutes.Name = "Intraday: Number of Minutes";
        Input_NumberOfMinutes.SetInt(15);

        Input_RoundExt.Name = "Round Extensions to TickSize";
        Input_RoundExt.SetYesNo(1);

        Input_NumberDaysToCalculate.Name = "Number of Days to Calculate";
        Input_NumberDaysToCalculate.SetInt(5);
        Input_NumberDaysToCalculate.SetIntLimits(1, INT_MAX);

        Input_StartEndTimeMethod.Name = "Start End Time Method";
        Input_StartEndTimeMethod.SetCustomInputStrings("Use Start/End Time;Use Session Start Time and Minutes From Start");
        Input_StartEndTimeMethod.SetCustomInputIndex(0);

        Input_PeriodEndAsMinutesFromSessionStart.Name = "Period End As Minutes from Session Start";
        Input_PeriodEndAsMinutesFromSessionStart.SetInt(30);

        Input_Multiplier1.Name = "Extension Multiplier 1";
        Input_Multiplier1.SetFloat(.5f);
        Input_Multiplier2.Name = "Extension Multiplier 2";
        Input_Multiplier2.SetFloat(1.0f);
        Input_Multiplier3.Name = "Extension Multiplier 3";
        Input_Multiplier3.SetFloat(1.5f);
        Input_Multiplier4.Name = "Extension Multiplier 4";
        Input_Multiplier4.SetFloat(2.0f);
        Input_Multiplier5.Name = "Extension Multiplier 5";
        Input_Multiplier5.SetFloat(2.5f);
        Input_Multiplier6.Name = "Extension Multiplier 6";
        Input_Multiplier6.SetFloat(3.0f);

        // NEW Inputs
        Input_ShowAdaptiveTargets.Name = "Show Adaptive Targets";
        Input_ShowAdaptiveTargets.SetYesNo(1);

        Input_TargetPercentOfIBRange.Name = "Target % of IB Range";
        Input_TargetPercentOfIBRange.SetFloat(50.0f);
        Input_TargetPercentOfIBRange.SetFloatLimits(1.0f, 500.0f);

        Input_TargetDisplayMode.Name = "Target Display (0=Adaptive, 1=Extended)";
        Input_TargetDisplayMode.SetInt(0);

        Input_TargetSource.Name = "Target Cross Source (0=Close, 1=Highs/Lows)";
        Input_TargetSource.SetInt(0);

        Input_ShowSignals.Name = "Show Bull/Bear Signals";
        Input_ShowSignals.SetYesNo(1);

        Input_SignalBiasMode.Name = "Signal Bias (0=None, 1=Midline Bias)";
        Input_SignalBiasMode.SetInt(0);

        Input_Debug.Name = "Debug Logging";
        Input_Debug.SetYesNo(0);

        // NEW: configurable signal offset
        Input_SignalOffsetTicks.Name = "Signal Offset (Ticks)";
        Input_SignalOffsetTicks.SetFloat(2.0f);
        Input_SignalOffsetTicks.SetFloatLimits(0.0f, 200.0f);

        sc.SetChartStudyTransparencyLevel(sc.ChartNumber, sc.StudyGraphInstanceID, 85);

        return;
    }

    // Persistent variables (existing)
    auto& r_PeriodFirstIndex = sc.GetPersistentInt(1);
    auto& r_PeriodConfirmed = sc.GetPersistentInt(2);

    auto& r_PeriodStartDateTime = sc.GetPersistentSCDateTime(1);
    auto& r_PeriodEndDateTime   = sc.GetPersistentSCDateTime(2);

    auto& r_PeriodHigh       = sc.GetPersistentFloat(1);
    auto& r_PeriodLow        = sc.GetPersistentFloat(2);
    auto& r_PeriodMid        = sc.GetPersistentFloat(3);
    auto& r_PeriodHighExt1   = sc.GetPersistentFloat(4);
    auto& r_PeriodHighExt2   = sc.GetPersistentFloat(5);
    auto& r_PeriodHighExt3   = sc.GetPersistentFloat(6);
    auto& r_PeriodHighExt4   = sc.GetPersistentFloat(7);
    auto& r_PeriodHighExt5   = sc.GetPersistentFloat(8);
    auto& r_PeriodHighExt6   = sc.GetPersistentFloat(9);
    auto& r_PeriodLowExt1    = sc.GetPersistentFloat(10);
    auto& r_PeriodLowExt2    = sc.GetPersistentFloat(11);
    auto& r_PeriodLowExt3    = sc.GetPersistentFloat(12);
    auto& r_PeriodLowExt4    = sc.GetPersistentFloat(13);
    auto& r_PeriodLowExt5    = sc.GetPersistentFloat(14);
    auto& r_PeriodLowExt6    = sc.GetPersistentFloat(15);

    // NEW persistent variables for adaptive targets/signals
    auto& r_HST = sc.GetPersistentFloat(30);
    auto& r_LST = sc.GetPersistentFloat(31);
    auto& r_UpCheck = sc.GetPersistentInt(30);
    auto& r_DownCheck = sc.GetPersistentInt(31);

    const bool ShowAdaptiveTargets = Input_ShowAdaptiveTargets.GetYesNo() != 0;
    const float TargetPercent = Input_TargetPercentOfIBRange.GetFloat() * 0.01f;
    const int TargetDisplayMode = Input_TargetDisplayMode.GetInt(); // 0 adaptive, 1 extended
    const int TargetSourceMode = Input_TargetSource.GetInt();       // 0 close, 1 highs/lows
    const bool ShowSignals = Input_ShowSignals.GetYesNo() != 0;
    const int BiasMode = Input_SignalBiasMode.GetInt();             // 0 none, 1 midline bias
    const bool Debug = Input_Debug.GetYesNo() != 0;

    // NEW: configurable signal offset
    const float SignalOffsetTicks = Input_SignalOffsetTicks.GetFloat();

    // Reset persistent variables upon full calculation
    if (sc.IsFullRecalculation && sc.Index == 0)
    {
        r_PeriodFirstIndex = -1;
        r_PeriodStartDateTime = 0;
        r_PeriodEndDateTime   = 0;
        r_PeriodHigh = -FLT_MAX;
        r_PeriodLow  =  FLT_MAX;

        // reset new state
        r_HST = 0.0f;
        r_LST = 0.0f;
        r_UpCheck = 1;
        r_DownCheck = 1;
    }

    SCDateTimeMS LastBarDateTime = sc.BaseDateTimeIn[sc.ArraySize - 1];
    SCDateTimeMS FirstCalculationDate = LastBarDateTime.GetDate() - SCDateTime::DAYS(Input_NumberDaysToCalculate.GetInt() - 1);

    SCDateTimeMS CurrentBarDateTime = sc.BaseDateTimeIn[sc.Index];

    SCDateTimeMS PrevBarDateTime;
    if (sc.Index > 0)
        PrevBarDateTime = sc.BaseDateTimeIn[sc.Index - 1];

    if (CurrentBarDateTime.GetDate() < FirstCalculationDate)
        return;

    bool Daily  = Input_IBType.GetIndex() == 0;
    bool Weekly = Input_IBType.GetIndex() == 1 || Input_IBType.GetIndex() == 2;
    bool Intraday = Input_IBType.GetIndex() == 3;
    bool IncludeSunday = Input_IBType.GetIndex() == 2;

    SCDateTimeMS StartDateTime = CurrentBarDateTime;

    if (Input_StartEndTimeMethod.GetIndex() == 0)
        StartDateTime.SetTime(Input_StartTime.GetTime());
    else
        StartDateTime.SetTime(sc.StartTimeOfDay);

    if (Weekly)
    {
        int PeriodStartDayOfWeek = IncludeSunday ? SUNDAY : MONDAY;

        int DayOfWeek = StartDateTime.GetDayOfWeek();

        if (DayOfWeek != PeriodStartDayOfWeek)
            StartDateTime.AddDays(7 - DayOfWeek + PeriodStartDayOfWeek);
    }

    if (PrevBarDateTime < StartDateTime && CurrentBarDateTime >= StartDateTime)
    {
        r_PeriodFirstIndex = sc.Index;
        r_PeriodConfirmed = 0;
        r_PeriodHigh = -FLT_MAX;
        r_PeriodLow  = FLT_MAX;

        r_PeriodStartDateTime = StartDateTime;
        r_PeriodEndDateTime = r_PeriodStartDateTime;

        if (Input_StartEndTimeMethod.GetIndex() == 0)
            r_PeriodEndDateTime.SetTime(Input_EndTime.GetTime());
        else
        {
            r_PeriodEndDateTime.SetTime(static_cast<int>(sc.StartTimeOfDay + Input_PeriodEndAsMinutesFromSessionStart.GetInt() * SECONDS_PER_MINUTE - 1));
        }

        if (Daily || Intraday)
        {
            if (SCDateTimeMS(r_PeriodEndDateTime) <= r_PeriodStartDateTime)
                r_PeriodEndDateTime.AddDays(1);
        }
        else if (Weekly)
        {
            int PeriodEndDayOfWeek = IncludeSunday ? Input_NumDays.GetInt() - 1 : Input_NumDays.GetInt();

            int DayOfWeek = r_PeriodEndDateTime.GetDayOfWeek();

            if (DayOfWeek != PeriodEndDayOfWeek)
                r_PeriodEndDateTime.AddDays(PeriodEndDayOfWeek - DayOfWeek);
        }
    }

    // Check end of period
    if (r_PeriodFirstIndex >= 0)
    {
        if (Intraday)
        {
            SCDateTimeMS IntradayEndDateTime = r_PeriodStartDateTime + SCDateTime::MINUTES(Input_NumberOfMinutes.GetInt()) - SCDateTime::MICROSECONDS(1);

            if (PrevBarDateTime < IntradayEndDateTime && CurrentBarDateTime > IntradayEndDateTime)
            {
                r_PeriodFirstIndex = sc.Index;
                r_PeriodConfirmed = 0;
                r_PeriodStartDateTime.AddMinutes(Input_NumberOfMinutes.GetInt());
                r_PeriodHigh = -FLT_MAX;
                r_PeriodLow  = FLT_MAX;
            }
        }

        if (CurrentBarDateTime > r_PeriodEndDateTime)
        {
            r_PeriodFirstIndex = -1;

            if (Intraday)
            {
                r_PeriodHigh = -FLT_MAX;
                r_PeriodLow  = FLT_MAX;
            }
        }
    }

    // Collecting data, back propagate if changed
    bool IsDeveloping = false;
    if (r_PeriodFirstIndex >= 0)
    {
        IsDeveloping = true;
        bool Changed = false;

        if (sc.High[sc.Index] > r_PeriodHigh)
        {
            r_PeriodHigh = sc.High[sc.Index];
            Changed = true;
        }

        if (sc.Low[sc.Index] < r_PeriodLow)
        {
            r_PeriodLow = sc.Low[sc.Index];
            Changed = true;
        }

        if (Changed)
        {
            r_PeriodMid = (r_PeriodHigh + r_PeriodLow) / 2.0f;

            float Range = r_PeriodHigh - r_PeriodLow;

            r_PeriodHighExt1 = r_PeriodHigh + Input_Multiplier1.GetFloat() * Range;
            r_PeriodHighExt2 = r_PeriodHigh + Input_Multiplier2.GetFloat() * Range;
            r_PeriodHighExt3 = r_PeriodHigh + Input_Multiplier3.GetFloat() * Range;
            r_PeriodHighExt4 = r_PeriodHigh + Input_Multiplier4.GetFloat() * Range;
            r_PeriodHighExt5 = r_PeriodHigh + Input_Multiplier5.GetFloat() * Range;
            r_PeriodHighExt6 = r_PeriodHigh + Input_Multiplier6.GetFloat() * Range;

            r_PeriodLowExt1 = r_PeriodLow - Input_Multiplier1.GetFloat() * Range;
            r_PeriodLowExt2 = r_PeriodLow - Input_Multiplier2.GetFloat() * Range;
            r_PeriodLowExt3 = r_PeriodLow - Input_Multiplier3.GetFloat() * Range;
            r_PeriodLowExt4 = r_PeriodLow - Input_Multiplier4.GetFloat() * Range;
            r_PeriodLowExt5 = r_PeriodLow - Input_Multiplier5.GetFloat() * Range;
            r_PeriodLowExt6 = r_PeriodLow - Input_Multiplier6.GetFloat() * Range;

            if (Input_RoundExt.GetYesNo())
            {
                r_PeriodHighExt1 = sc.RoundToTickSize(r_PeriodHighExt1, sc.TickSize);
                r_PeriodHighExt2 = sc.RoundToTickSize(r_PeriodHighExt2, sc.TickSize);
                r_PeriodHighExt3 = sc.RoundToTickSize(r_PeriodHighExt3, sc.TickSize);
                r_PeriodHighExt4 = sc.RoundToTickSize(r_PeriodHighExt4, sc.TickSize);
                r_PeriodHighExt5 = sc.RoundToTickSize(r_PeriodHighExt5, sc.TickSize);
                r_PeriodHighExt6 = sc.RoundToTickSize(r_PeriodHighExt6, sc.TickSize);

                r_PeriodLowExt1 = sc.RoundToTickSize(r_PeriodLowExt1, sc.TickSize);
                r_PeriodLowExt2 = sc.RoundToTickSize(r_PeriodLowExt2, sc.TickSize);
                r_PeriodLowExt3 = sc.RoundToTickSize(r_PeriodLowExt3, sc.TickSize);
                r_PeriodLowExt4 = sc.RoundToTickSize(r_PeriodLowExt4, sc.TickSize);
                r_PeriodLowExt5 = sc.RoundToTickSize(r_PeriodLowExt5, sc.TickSize);
                r_PeriodLowExt6 = sc.RoundToTickSize(r_PeriodLowExt6, sc.TickSize);
            }

            // Backfill any changes to IB region
            for (int Index = r_PeriodFirstIndex; Index < sc.Index; Index++)
            {
                Subgraph_IBRegionTop[Index] = r_PeriodHigh;
                Subgraph_IBRegionBottom[Index] = r_PeriodLow;
            }

            sc.EarliestUpdateSubgraphDataArrayIndex = r_PeriodFirstIndex;
        }
    }

    // Plot current values
    if (r_PeriodLow != FLT_MAX)
    {
        if (IsDeveloping)
        {
            Subgraph_IBRegionTop[sc.Index] = r_PeriodHigh;
            Subgraph_IBRegionBottom[sc.Index] = r_PeriodLow;
        }
        else
        {
            if (!r_PeriodConfirmed)
            {
                // First call after IB confirmed: initialize adaptive tracking anchors
                const float ibRange = r_PeriodHigh - r_PeriodLow;
                const float step = ibRange * TargetPercent;

                r_HST = r_PeriodHigh + step;
                r_LST = r_PeriodLow - step;
                r_UpCheck = 1;
                r_DownCheck = 1;

                // Backfill IB levels
                for (int Index = sc.EarliestUpdateSubgraphDataArrayIndex; Index < sc.Index; Index++)
                {
                    Subgraph_IBHigh[Index]  = r_PeriodHigh;
                    Subgraph_IBLow[Index]   = r_PeriodLow;
                    Subgraph_IBMid[Index]   = r_PeriodMid;
                    Subgraph_IBHExt1[Index] = r_PeriodHighExt1;
                    Subgraph_IBHExt2[Index] = r_PeriodHighExt2;
                    Subgraph_IBHExt3[Index] = r_PeriodHighExt3;
                    Subgraph_IBHExt4[Index] = r_PeriodHighExt4;
                    Subgraph_IBHExt5[Index] = r_PeriodHighExt5;
                    Subgraph_IBHExt6[Index] = r_PeriodHighExt6;
                    Subgraph_IBLExt1[Index] = r_PeriodLowExt1;
                    Subgraph_IBLExt2[Index] = r_PeriodLowExt2;
                    Subgraph_IBLExt3[Index] = r_PeriodLowExt3;
                    Subgraph_IBLExt4[Index] = r_PeriodLowExt4;
                    Subgraph_IBLExt5[Index] = r_PeriodLowExt5;
                    Subgraph_IBLExt6[Index] = r_PeriodLowExt6;
                }
                r_PeriodConfirmed = 1;
            }

            Subgraph_IBHigh[sc.Index]  = r_PeriodHigh;
            Subgraph_IBLow[sc.Index]   = r_PeriodLow;
            Subgraph_IBMid[sc.Index]   = r_PeriodMid;
            Subgraph_IBHExt1[sc.Index] = r_PeriodHighExt1;
            Subgraph_IBHExt2[sc.Index] = r_PeriodHighExt2;
            Subgraph_IBHExt3[sc.Index] = r_PeriodHighExt3;
            Subgraph_IBHExt4[sc.Index] = r_PeriodHighExt4;
            Subgraph_IBHExt5[sc.Index] = r_PeriodHighExt5;
            Subgraph_IBHExt6[sc.Index] = r_PeriodHighExt6;
            Subgraph_IBLExt1[sc.Index] = r_PeriodLowExt1;
            Subgraph_IBLExt2[sc.Index] = r_PeriodLowExt2;
            Subgraph_IBLExt3[sc.Index] = r_PeriodLowExt3;
            Subgraph_IBLExt4[sc.Index] = r_PeriodLowExt4;
            Subgraph_IBLExt5[sc.Index] = r_PeriodLowExt5;
            Subgraph_IBLExt6[sc.Index] = r_PeriodLowExt6;

            // Adaptive targets + signals
            SG_SignalUp[sc.Index] = 0.0f;
            SG_SignalDn[sc.Index] = 0.0f;

            SG_TUp1[sc.Index] = SG_TUp2[sc.Index] = SG_TUp3[sc.Index] = SG_TUp4[sc.Index] = SG_TUp5[sc.Index] = SG_TUp6[sc.Index] = 0.0f;
            SG_TDn1[sc.Index] = SG_TDn2[sc.Index] = SG_TDn3[sc.Index] = SG_TDn4[sc.Index] = SG_TDn5[sc.Index] = SG_TDn6[sc.Index] = 0.0f;

            const float ibRange = r_PeriodHigh - r_PeriodLow;
            const float step = ibRange * TargetPercent;

            if (ShowAdaptiveTargets && step > 0.0f)
            {
                const float h_src = (TargetSourceMode == 0 ? (float)sc.Close[sc.Index] : (float)sc.High[sc.Index]);
                const float l_src = (TargetSourceMode == 0 ? (float)sc.Close[sc.Index] : (float)sc.Low[sc.Index]);

                if (h_src > r_HST) r_HST = h_src;
                if (l_src < r_LST) r_LST = l_src;

                const int up_max = Get1Up((r_HST - r_PeriodHigh) / step);
                const int down_max = Get1Up((r_PeriodLow - r_LST) / step);

                const int up_cur = (int)fmax(0.0, (double)Get1Up((h_src - r_PeriodHigh) / step));
                const int down_cur = (int)fmax(0.0, (double)Get1Up((r_PeriodLow - l_src) / step));

                auto ShouldShowUpLevel = [&](int lvl) -> bool
                {
                    if (lvl < 1 || lvl > 6) return false;
                    if (TargetDisplayMode == 1) return lvl <= up_max; // Extended
                    return lvl <= up_cur && lvl >= (up_cur - 2);      // Adaptive
                };

                auto ShouldShowDnLevel = [&](int lvl) -> bool
                {
                    if (lvl < 1 || lvl > 6) return false;
                    if (TargetDisplayMode == 1) return lvl <= down_max; // Extended
                    return lvl <= down_cur && lvl >= (down_cur - 2);
                };

                if (ShouldShowUpLevel(1)) SG_TUp1[sc.Index] = r_PeriodHigh + step * 1;
                if (ShouldShowUpLevel(2)) SG_TUp2[sc.Index] = r_PeriodHigh + step * 2;
                if (ShouldShowUpLevel(3)) SG_TUp3[sc.Index] = r_PeriodHigh + step * 3;
                if (ShouldShowUpLevel(4)) SG_TUp4[sc.Index] = r_PeriodHigh + step * 4;
                if (ShouldShowUpLevel(5)) SG_TUp5[sc.Index] = r_PeriodHigh + step * 5;
                if (ShouldShowUpLevel(6)) SG_TUp6[sc.Index] = r_PeriodHigh + step * 6;

                if (ShouldShowDnLevel(1)) SG_TDn1[sc.Index] = r_PeriodLow - step * 1;
                if (ShouldShowDnLevel(2)) SG_TDn2[sc.Index] = r_PeriodLow - step * 2;
                if (ShouldShowDnLevel(3)) SG_TDn3[sc.Index] = r_PeriodLow - step * 3;
                if (ShouldShowDnLevel(4)) SG_TDn4[sc.Index] = r_PeriodLow - step * 4;
                if (ShouldShowDnLevel(5)) SG_TDn5[sc.Index] = r_PeriodLow - step * 5;
                if (ShouldShowDnLevel(6)) SG_TDn6[sc.Index] = r_PeriodLow - step * 6;

                // Signals: cross above IBH / cross below IBL
                if (ShowSignals && sc.Index > 0)
                {
                    const float prevClose = (float)sc.Close[sc.Index - 1];
                    const float curClose  = (float)sc.Close[sc.Index];

                    const bool xup = prevClose <= r_PeriodHigh && curClose > r_PeriodHigh;
                    const bool xdn = prevClose >= r_PeriodLow  && curClose < r_PeriodLow;

                    const bool allowUp = (BiasMode == 0) ? true : (curClose >= r_PeriodMid);
                    const bool allowDn = (BiasMode == 0) ? true : (curClose <= r_PeriodMid);

                    const float offset = (float)(SignalOffsetTicks * sc.TickSize);

                    if (xup && allowUp)
                        SG_SignalUp[sc.Index] = (float)sc.Low[sc.Index] - offset;   // below candle

                    if (xdn && allowDn)
                        SG_SignalDn[sc.Index] = (float)sc.High[sc.Index] + offset;  // above candle
                }
            }
        }
    }
}