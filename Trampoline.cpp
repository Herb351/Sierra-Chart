#include "sierrachart.h"

SCDLLName("TrampolineStudy")

SCSFExport scsf_Trampoline(SCStudyInterfaceRef sc)
{
    // ============================
    // Inputs
    // ============================
    SCInputRef In_RsiLength        = sc.Input[0];
    SCInputRef In_BBWidthThreshold = sc.Input[1];
    SCInputRef In_TrampRsiLow      = sc.Input[2];
    SCInputRef In_TrampRsiHigh     = sc.Input[3];
    SCInputRef In_BBLength         = sc.Input[4];
    SCInputRef In_BBStdDev         = sc.Input[5];

    SCInputRef In_ShowTRText = sc.Input[6];
    SCInputRef In_TRFontSize = sc.Input[7];

    // Region 0 offset should be in *ticks* (price units), not RSI points
    SCInputRef In_TROffsetTicks = sc.Input[8];

    // Where to place the TR relative to bar
    SCInputRef In_TRAboveOffsetTicks = sc.Input[9]; // for down thrust (above bar)
    SCInputRef In_TRBelowOffsetTicks = sc.Input[10]; // for up thrust (below bar)

    // ============================
    // Subgraphs (internals)
    // ============================
    SCSubgraphRef SG_BB      = sc.Subgraph[0]; // Bollinger arrays
    SCSubgraphRef SG_RSI_Eng = sc.Subgraph[1]; // RSI used by Trampoline

    if (sc.SetDefaults)
    {
        sc.GraphName = "Trampoline (Text Only, Region 0)";
        sc.StudyDescription =
            "Standalone Trampoline logic. Draws historical 'TR' text markers on bar close in Region 0.";

        sc.AutoLoop     = 1;
        sc.UpdateAlways = 0;
        sc.DrawZeros    = 0;

        // Main price graph
        sc.GraphRegion = 0;

        // Inputs
        In_RsiLength.Name = "RSI Length";
        In_RsiLength.SetInt(14);
        In_RsiLength.SetIntLimits(1, 2000);

        In_BBWidthThreshold.Name = "BB Width Threshold";
        In_BBWidthThreshold.SetFloat(0.0015f);

        In_TrampRsiLow.Name = "RSI Lower Threshold";
        In_TrampRsiLow.SetInt(25);
        In_TrampRsiLow.SetIntLimits(0, 100);

        In_TrampRsiHigh.Name = "RSI Upper Threshold";
        In_TrampRsiHigh.SetInt(72);
        In_TrampRsiHigh.SetIntLimits(0, 100);

        In_BBLength.Name = "BB Length";
        In_BBLength.SetInt(20);
        In_BBLength.SetIntLimits(1, 500);

        In_BBStdDev.Name = "BB StdDev";
        In_BBStdDev.SetFloat(2.0f);

        In_ShowTRText.Name = "Show TR text markers";
        In_ShowTRText.SetYesNo(1);

        In_TRFontSize.Name = "TR font size";
        In_TRFontSize.SetInt(9);
        In_TRFontSize.SetIntLimits(6, 30);

        In_TROffsetTicks.Name = "Extra vertical offset (ticks)";
        In_TROffsetTicks.SetFloat(0.0f);

        In_TRAboveOffsetTicks.Name = "Down-thrust TR above bar (ticks)";
        In_TRAboveOffsetTicks.SetFloat(2.0f);

        In_TRBelowOffsetTicks.Name = "Up-thrust TR below bar (ticks)";
        In_TRBelowOffsetTicks.SetFloat(2.0f);

        // Internals
        SG_BB.DrawStyle      = DRAWSTYLE_IGNORE;
        SG_RSI_Eng.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;

    // Pine-like confirmed bar behavior
    const bool barClosed = (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_CLOSED);
    if (!barClosed)
        return;

    if (i < 1)
        return;

    // ===== Helper: draw "TR" text in Region 0, historical per bar =====
    // We purposely make LineNumber unique per bar+direction, so old bars remain.
    auto DrawTR_Region0 = [&](int BarIndex, float Price, int Direction /*1=UpThrust(below), 2=DownThrust(above)*/)
    {
        s_UseTool Tool;
        Tool.Clear();
        Tool.ChartNumber = sc.ChartNumber;
        Tool.DrawingType = DRAWING_TEXT;
        Tool.AddMethod   = UTAM_ADD_OR_ADJUST;

        const int LN_INSTANCE_STRIDE = 1000000;
        const int LN_BAR_STRIDE      = 10;

        Tool.LineNumber = sc.StudyGraphInstanceID * LN_INSTANCE_STRIDE + BarIndex * LN_BAR_STRIDE + Direction;

        Tool.BeginIndex = BarIndex;
        Tool.Region     = 0;
        Tool.BeginValue = Price;

        Tool.TextAlignment = DT_CENTER | DT_VCENTER;
        Tool.Color         = RGB(255, 255, 255);
        Tool.FontSize      = In_TRFontSize.GetInt();
        Tool.FontBold      = 1;

        Tool.FontBackColor = RGB(0, 0, 0);
        Tool.TransparentLabelBackground = 1;

        Tool.Text = "TR";
        sc.UseTool(Tool);
    };

    // ===== Engine RSI for Trampoline =====
    const int Len = In_RsiLength.GetInt();
    sc.RSI(sc.Close, SG_RSI_Eng, MOVAVGTYPE_SIMPLE, Len);

    // ===== Trampoline =====
    if (i >= 10)
    {
        const int   bbLen    = In_BBLength.GetInt();
        const float bbMult   = In_BBStdDev.GetFloat();
        const float bbThresh = In_BBWidthThreshold.GetFloat();
        const float rsiLow   = (float)In_TrampRsiLow.GetInt();
        const float rsiHigh  = (float)In_TrampRsiHigh.GetInt();

        sc.BollingerBands(sc.Close, SG_BB, bbLen, bbMult, MOVAVGTYPE_SIMPLE);

        auto IsRed   = [&](int idx) { return sc.Close[idx] < sc.Open[idx]; };
        auto IsGreen = [&](int idx) { return sc.Close[idx] > sc.Open[idx]; };

        auto BBWAt = [&](int idx) -> float
        {
            const float b = SG_BB.Arrays[2][idx]; // basis/middle
            return (b == 0.0f) ? 0.0f : (SG_BB.Arrays[0][idx] - SG_BB.Arrays[1][idx]) / b;
        };

        auto BackAt = [&](int idx) -> bool
        {
            return IsRed(idx)
                && SG_RSI_Eng[idx] <= rsiLow
                && sc.Close[idx] < SG_BB.Arrays[1][idx]
                && BBWAt(idx) > bbThresh;
        };

        auto ForAt = [&](int idx) -> bool
        {
            return IsGreen(idx)
                && SG_RSI_Eng[idx] >= rsiHigh
                && sc.Close[idx] > SG_BB.Arrays[0][idx]
                && BBWAt(idx) > bbThresh;
        };

        auto WeGoUpAt = [&](int idx) -> bool
        {
            if (idx < 10) return false;
            return IsGreen(idx)
                && (BackAt(idx - 1) || BackAt(idx - 2) || BackAt(idx - 3) || BackAt(idx - 4) || BackAt(idx - 5))
                && sc.High[idx] > sc.High[idx - 1];
        };

        auto WeGoDownAt = [&](int idx) -> bool
        {
            if (idx < 10) return false;
            return IsRed(idx)
                && (ForAt(idx - 1) || ForAt(idx - 2) || ForAt(idx - 3) || ForAt(idx - 4) || ForAt(idx - 5))
                && sc.Low[idx] < sc.Low[idx - 1];
        };

        const bool upThrust =
            WeGoUpAt(i) && !WeGoUpAt(i - 1) && !WeGoUpAt(i - 2) && !WeGoUpAt(i - 3) && !WeGoUpAt(i - 4);

        const bool downThrust =
            WeGoDownAt(i) && !WeGoDownAt(i - 1) && !WeGoDownAt(i - 2) && !WeGoDownAt(i - 3) && !WeGoDownAt(i - 4);

        if (In_ShowTRText.GetYesNo())
        {
            const float tick = (float)sc.TickSize;
            const float extra = In_TROffsetTicks.GetFloat() * tick;

            if (upThrust)
            {
                // Below bar (use low)
                const float below = (In_TRBelowOffsetTicks.GetFloat() * tick) + extra;
                DrawTR_Region0(i, (float)sc.Low[i] - below, 1);
            }

            if (downThrust)
            {
                // Above bar (use high)
                const float above = (In_TRAboveOffsetTicks.GetFloat() * tick) + extra;
                DrawTR_Region0(i, (float)sc.High[i] + above, 2);
            }
        }
    }
}