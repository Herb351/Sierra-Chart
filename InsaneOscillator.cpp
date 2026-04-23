#include "sierrachart.h"

SCDLLName("InsaneOscillator")

SCSFExport scsf_InsaneOscillator(SCStudyInterfaceRef sc)
{
    // ============================
    // Inputs
    // ============================
    SCInputRef In_RsiLength = sc.Input[0];
    SCInputRef In_RsiOB     = sc.Input[1];
    SCInputRef In_RsiOS     = sc.Input[2];

    // Trampoline
    SCInputRef In_ShowTramp        = sc.Input[3];
    SCInputRef In_BBWidthThreshold = sc.Input[4];
    SCInputRef In_TrampRsiLow      = sc.Input[5];
    SCInputRef In_TrampRsiHigh     = sc.Input[6];
    SCInputRef In_BBLength         = sc.Input[7];
    SCInputRef In_BBStdDev         = sc.Input[8];

    // TR text
    SCInputRef In_ShowTRText = sc.Input[9];
    SCInputRef In_TRFontSize = sc.Input[10];
    SCInputRef In_TROffset   = sc.Input[11];

    // Squeeze Relaxer v2.1 inputs
    SCInputRef In_ShowSqueeze = sc.Input[12];
    SCInputRef In_SqTolerance = sc.Input[13];
    SCInputRef In_AdxSqueeze  = sc.Input[14];
    SCInputRef In_AdxLen      = sc.Input[15];
    SCInputRef In_DiLen       = sc.Input[16];

    // Bollinger Bands wicking (John Wick BB) -> drawn via UseTool text "B"
    SCInputRef In_ShowBBWicking = sc.Input[17];
    SCInputRef In_WickBBLength  = sc.Input[18];
    SCInputRef In_WickBBStdDev  = sc.Input[19];
    SCInputRef In_BFontSize     = sc.Input[20];
    SCInputRef In_BOffset       = sc.Input[21];

    // WAE (Waddah Attar Explosion)
    SCInputRef In_ShowWAE        = sc.Input[22];
    SCInputRef In_WAEThickness   = sc.Input[23];
    SCInputRef In_WAEThreshold   = sc.Input[24];
    SCInputRef In_WAESensitivity = sc.Input[25];
    SCInputRef In_WAEFastLen     = sc.Input[26];
    SCInputRef In_WAESlowLen     = sc.Input[27];

    // Divergence (RSI)
    SCInputRef In_ShowDivergence   = sc.Input[28];
    SCInputRef In_DivLookbackLeft  = sc.Input[29];
    SCInputRef In_DivLookbackRight = sc.Input[30];
    SCInputRef In_DivRangeUpper    = sc.Input[31];
    SCInputRef In_DivRangeLower    = sc.Input[32];
    SCInputRef In_DivFontSize      = sc.Input[33];

    // EMA fast/slow trend meter (9/21 cross by default)
    SCInputRef In_ShowTrendMeter = sc.Input[34];
    SCInputRef In_TrendFastLen   = sc.Input[35];
    SCInputRef In_TrendSlowLen   = sc.Input[36];
    SCInputRef In_TrendLevel     = sc.Input[37];
    SCInputRef In_TrendThickness = sc.Input[38];

    // Bull Rush dashboard meter
    SCInputRef In_ShowBullRushMeter = sc.Input[39];
    SCInputRef In_BullRushLevel     = sc.Input[40];
    SCInputRef In_BullRushThickness = sc.Input[41];

    // ============================
    // Subgraphs
    // ============================
    SCSubgraphRef SG_RSI_Pine = sc.Subgraph[0];
    SCSubgraphRef SG_Midline  = sc.Subgraph[1];

    SCSubgraphRef SG_UpRMA   = sc.Subgraph[2];
    SCSubgraphRef SG_DownRMA = sc.Subgraph[3];

    // Trampoline internals
    SCSubgraphRef SG_TrampUp = sc.Subgraph[4];
    SCSubgraphRef SG_TrampDn = sc.Subgraph[5];
    SCSubgraphRef SG_BB      = sc.Subgraph[6];
    SCSubgraphRef SG_RSI_Eng = sc.Subgraph[7];

    // Squeeze markers
    SCSubgraphRef SG_SQZ_Buy  = sc.Subgraph[8];
    SCSubgraphRef SG_SQZ_Sell = sc.Subgraph[9];

    // ADX internals
    SCSubgraphRef SG_ADX_trRMA  = sc.Subgraph[10];
    SCSubgraphRef SG_ADX_pRMA   = sc.Subgraph[11];
    SCSubgraphRef SG_ADX_mRMA   = sc.Subgraph[12];
    SCSubgraphRef SG_ADX_adxIn  = sc.Subgraph[13];
    SCSubgraphRef SG_ADX_adxRMA = sc.Subgraph[14];

    // Squeeze internals
    SCSubgraphRef SG_SQZ_avg2   = sc.Subgraph[15];
    SCSubgraphRef SG_SQZ_val    = sc.Subgraph[16];
    SCSubgraphRef SG_SQZ_cRed   = sc.Subgraph[17];
    SCSubgraphRef SG_SQZ_cGreen = sc.Subgraph[18];
    SCSubgraphRef SG_SQZ_pos    = sc.Subgraph[19];
    SCSubgraphRef SG_SQZ_neg    = sc.Subgraph[20];

    // Wick BB internal
    SCSubgraphRef SG_WickBB = sc.Subgraph[21];

    // WAE
    SCSubgraphRef SG_WAE_Line     = sc.Subgraph[22];
    SCSubgraphRef SG_WAE_FastEMA  = sc.Subgraph[23];
    SCSubgraphRef SG_WAE_SlowEMA  = sc.Subgraph[24];
    SCSubgraphRef SG_WAE_MacdDiff = sc.Subgraph[25];

    // Divergence internals
    SCSubgraphRef SG_DivLastPivotLowIndex  = sc.Subgraph[26];
    SCSubgraphRef SG_DivLastPivotHighIndex = sc.Subgraph[27];

    // EMA Trend Meter
    SCSubgraphRef SG_TrendMeter   = sc.Subgraph[28];
    SCSubgraphRef SG_TrendFastEMA = sc.Subgraph[29];
    SCSubgraphRef SG_TrendSlowEMA = sc.Subgraph[30];

    // Bull Rush Meter plotted
    SCSubgraphRef SG_BullRushMeter = sc.Subgraph[31];

    // ============================
    // Colors
    // ============================
    const COLORREF CLR_UP  = RGB(0, 200, 0);
    const COLORREF CLR_DN  = RGB(255, 0, 0);
    const COLORREF CLR_NEU = RGB(128, 128, 128);
    const COLORREF CLR_OFF = RGB(0, 0, 0);

    if (sc.SetDefaults)
    {
        sc.GraphName = "Insane Oscillator";
        sc.StudyDescription =
            "RSI (Pine-exact) + Trampoline (TR text) + Squeeze Relaxer v2.1 + BB wicks (B text) + WAE + Divergence "
            "+ EMA Trend Meter (Fast/Slow) + Bull Rush Meter. Bar-close-only coloring like Pine barstate.isconfirmed.";

        sc.AutoLoop     = 1;
        sc.UpdateAlways = 0;
        sc.GraphRegion  = 1;
        sc.DrawZeros    = 0;

        // ===== Inputs =====
        In_RsiLength.Name = "RSI Length";
        In_RsiLength.SetInt(14);
        In_RsiLength.SetIntLimits(1, 2000);

        In_RsiOB.Name = "RSI Overbought";
        In_RsiOB.SetFloat(70.0f);

        In_RsiOS.Name = "RSI Oversold";
        In_RsiOS.SetFloat(30.0f);

        In_ShowTramp.Name = "Show Trampoline";
        In_ShowTramp.SetYesNo(1);

        In_BBWidthThreshold.Name = "Trampoline: BB Width Threshold";
        In_BBWidthThreshold.SetFloat(0.0015f);

        In_TrampRsiLow.Name = "Trampoline: RSI Lower Threshold";
        In_TrampRsiLow.SetInt(25);

        In_TrampRsiHigh.Name = "Trampoline: RSI Upper Threshold";
        In_TrampRsiHigh.SetInt(72);

        In_BBLength.Name = "Trampoline: BB Length";
        In_BBLength.SetInt(20);
        In_BBLength.SetIntLimits(1, 500);

        In_BBStdDev.Name = "Trampoline: BB StdDev";
        In_BBStdDev.SetFloat(2.0f);

        In_ShowTRText.Name = "Show TR text markers (Region 1)";
        In_ShowTRText.SetYesNo(1);

        In_TRFontSize.Name = "TR font size";
        In_TRFontSize.SetInt(9);
        In_TRFontSize.SetIntLimits(6, 24);

        In_TROffset.Name = "TR vertical offset (RSI points)";
        In_TROffset.SetFloat(0.0f);

        In_ShowSqueeze.Name = "Show Squeeze Dot";
        In_ShowSqueeze.SetYesNo(1);

        In_SqTolerance.Name = "Squeeze Tolerance";
        In_SqTolerance.SetInt(2);
        In_SqTolerance.SetIntLimits(0, 100);

        In_AdxSqueeze.Name = "ADX Threshold";
        In_AdxSqueeze.SetInt(21);
        In_AdxSqueeze.SetIntLimits(0, 100);

        In_AdxLen.Name = "ADX Smoothing";
        In_AdxLen.SetInt(14);
        In_AdxLen.SetIntLimits(1, 500);

        In_DiLen.Name = "DI Length";
        In_DiLen.SetInt(14);
        In_DiLen.SetIntLimits(1, 500);

        In_ShowBBWicking.Name = "Show bollinger bands wicking";
        In_ShowBBWicking.SetYesNo(1);

        In_WickBBLength.Name = "Wicking BB Length";
        In_WickBBLength.SetInt(20);
        In_WickBBLength.SetIntLimits(1, 500);

        In_WickBBStdDev.Name = "Wicking BB StdDev";
        In_WickBBStdDev.SetFloat(2.5f);

        In_BFontSize.Name = "B font size";
        In_BFontSize.SetInt(9);
        In_BFontSize.SetIntLimits(6, 24);

        In_BOffset.Name = "B vertical offset (RSI points)";
        In_BOffset.SetFloat(0.0f);

        // WAE inputs
        In_ShowWAE.Name = "Show waddah explosion";
        In_ShowWAE.SetYesNo(1);

        In_WAEThickness.Name = "Waddah Explosion Thickness";
        In_WAEThickness.SetInt(2);
        In_WAEThickness.SetIntLimits(1, 10);

        In_WAEThreshold.Name = "Threshold to trigger";
        In_WAEThreshold.SetInt(120);
        In_WAEThreshold.SetIntLimits(0, 1000000);

        In_WAESensitivity.Name = "Sensitivity";
        In_WAESensitivity.SetInt(150);
        In_WAESensitivity.SetIntLimits(1, 1000000);

        In_WAEFastLen.Name = "FastEMA Length";
        In_WAEFastLen.SetInt(20);
        In_WAEFastLen.SetIntLimits(1, 2000);

        In_WAESlowLen.Name = "SlowEMA Length";
        In_WAESlowLen.SetInt(40);
        In_WAESlowLen.SetIntLimits(1, 2000);

        // Divergence inputs
        In_ShowDivergence.Name = "Show divergence labels";
        In_ShowDivergence.SetYesNo(0);

        In_DivLookbackLeft.Name = "Divergence Lookback Left";
        In_DivLookbackLeft.SetInt(5);
        In_DivLookbackLeft.SetIntLimits(1, 50);

        In_DivLookbackRight.Name = "Divergence Lookback Right";
        In_DivLookbackRight.SetInt(5);
        In_DivLookbackRight.SetIntLimits(1, 50);

        In_DivRangeUpper.Name = "Divergence Range Upper";
        In_DivRangeUpper.SetInt(60);
        In_DivRangeUpper.SetIntLimits(1, 5000);

        In_DivRangeLower.Name = "Divergence Range Lower";
        In_DivRangeLower.SetInt(5);
        In_DivRangeLower.SetIntLimits(0, 5000);

        In_DivFontSize.Name = "Divergence Font Size";
        In_DivFontSize.SetInt(9);
        In_DivFontSize.SetIntLimits(6, 24);

        // EMA Trend Meter
        In_ShowTrendMeter.Name = "Show EMA Trend Meter (Fast/Slow)";
        In_ShowTrendMeter.SetYesNo(1);

        In_TrendFastLen.Name = "EMA Trend Fast Length";
        In_TrendFastLen.SetInt(9);
        In_TrendFastLen.SetIntLimits(1, 2000);

        In_TrendSlowLen.Name = "EMA Trend Slow Length";
        In_TrendSlowLen.SetInt(21);
        In_TrendSlowLen.SetIntLimits(1, 2000);

        In_TrendLevel.Name = "EMA Trend Meter Level";
        In_TrendLevel.SetFloat(71.0f);

        In_TrendThickness.Name = "EMA Trend Meter Thickness";
        In_TrendThickness.SetInt(2);
        In_TrendThickness.SetIntLimits(1, 10);

        // Bull Rush
        In_ShowBullRushMeter.Name = "Show Bull Rush Meter";
        In_ShowBullRushMeter.SetYesNo(1);

        In_BullRushLevel.Name = "Bull Rush Level";
        In_BullRushLevel.SetFloat(30.0f);

        In_BullRushThickness.Name = "Bull Rush Thickness";
        In_BullRushThickness.SetInt(1);
        In_BullRushThickness.SetIntLimits(1, 10);

        // ===== Subgraph defaults =====
        SG_RSI_Pine.Name = "RSI (Pine-exact)";
        SG_RSI_Pine.DrawStyle = DRAWSTYLE_LINE;
        SG_RSI_Pine.PrimaryColor = CLR_NEU;
        SG_RSI_Pine.SecondaryColorUsed = 1;
        SG_RSI_Pine.LineWidth = 2;
        SG_RSI_Pine.DrawZeros = 0;

        SG_Midline.Name = "Midline (50)";
        SG_Midline.DrawStyle = DRAWSTYLE_LINE;
        SG_Midline.PrimaryColor = RGB(120, 120, 120);
        SG_Midline.LineWidth = 1;
        SG_Midline.DrawZeros = 0;

        // Internals ignore
        SG_UpRMA.DrawStyle = DRAWSTYLE_IGNORE;
        SG_DownRMA.DrawStyle = DRAWSTYLE_IGNORE;
        SG_TrampUp.DrawStyle = DRAWSTYLE_IGNORE;
        SG_TrampDn.DrawStyle = DRAWSTYLE_IGNORE;
        SG_BB.DrawStyle = DRAWSTYLE_IGNORE;
        SG_RSI_Eng.DrawStyle = DRAWSTYLE_IGNORE;

        SG_SQZ_Buy.Name = "Squeeze Buy";
        SG_SQZ_Buy.DrawStyle = DRAWSTYLE_DIAMOND;
        SG_SQZ_Buy.PrimaryColor = RGB(255, 255, 0);
        SG_SQZ_Buy.LineWidth = 6;
        SG_SQZ_Buy.DrawZeros = 0;

        SG_SQZ_Sell.Name = "Squeeze Sell";
        SG_SQZ_Sell.DrawStyle = DRAWSTYLE_DIAMOND;
        SG_SQZ_Sell.PrimaryColor = RGB(255, 255, 0);
        SG_SQZ_Sell.LineWidth = 6;
        SG_SQZ_Sell.DrawZeros = 0;

        SG_ADX_trRMA.DrawStyle  = DRAWSTYLE_IGNORE;
        SG_ADX_pRMA.DrawStyle   = DRAWSTYLE_IGNORE;
        SG_ADX_mRMA.DrawStyle   = DRAWSTYLE_IGNORE;
        SG_ADX_adxIn.DrawStyle  = DRAWSTYLE_IGNORE;
        SG_ADX_adxRMA.DrawStyle = DRAWSTYLE_IGNORE;

        SG_SQZ_avg2.DrawStyle   = DRAWSTYLE_IGNORE;
        SG_SQZ_val.DrawStyle    = DRAWSTYLE_IGNORE;
        SG_SQZ_cRed.DrawStyle   = DRAWSTYLE_IGNORE;
        SG_SQZ_cGreen.DrawStyle = DRAWSTYLE_IGNORE;
        SG_SQZ_pos.DrawStyle    = DRAWSTYLE_IGNORE;
        SG_SQZ_neg.DrawStyle    = DRAWSTYLE_IGNORE;

        SG_WickBB.DrawStyle = DRAWSTYLE_IGNORE;

        SG_WAE_Line.Name = "Waddah Explosion (50)";
        SG_WAE_Line.DrawStyle = DRAWSTYLE_LINE;
        SG_WAE_Line.PrimaryColor = CLR_NEU;
        SG_WAE_Line.SecondaryColorUsed = 1;
        SG_WAE_Line.LineWidth = 2;
        SG_WAE_Line.DrawZeros = 0;

        SG_WAE_FastEMA.DrawStyle  = DRAWSTYLE_IGNORE;
        SG_WAE_SlowEMA.DrawStyle  = DRAWSTYLE_IGNORE;
        SG_WAE_MacdDiff.DrawStyle = DRAWSTYLE_IGNORE;

        SG_DivLastPivotLowIndex.DrawStyle  = DRAWSTYLE_IGNORE;
        SG_DivLastPivotHighIndex.DrawStyle = DRAWSTYLE_IGNORE;

        SG_TrendMeter.Name = "EMA Trend Meter";
        SG_TrendMeter.DrawStyle = DRAWSTYLE_LINE;
        SG_TrendMeter.PrimaryColor = CLR_NEU;
        SG_TrendMeter.SecondaryColorUsed = 1;
        SG_TrendMeter.LineWidth = 2;
        SG_TrendMeter.DrawZeros = 0;

        SG_TrendFastEMA.DrawStyle = DRAWSTYLE_IGNORE;
        SG_TrendSlowEMA.DrawStyle = DRAWSTYLE_IGNORE;

        SG_BullRushMeter.Name = "Bull Rush Meter";
        SG_BullRushMeter.DrawStyle = DRAWSTYLE_LINE;
        SG_BullRushMeter.PrimaryColor = CLR_OFF;
        SG_BullRushMeter.SecondaryColorUsed = 1;
        SG_BullRushMeter.DrawZeros = 0;

        return;
    }

    const int i = sc.Index;

    // Pine-like confirmed bar behavior
    const bool barClosed = (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_CLOSED);

    // ===== Helper: draw "TR" text in Region 1 =====
    auto DrawTR_Region1 = [&](int BarIndex, float Value)
    {
        s_UseTool Tool;
        Tool.Clear();
        Tool.ChartNumber  = sc.ChartNumber;
        Tool.DrawingType  = DRAWING_TEXT;
        Tool.AddMethod    = UTAM_ADD_OR_ADJUST;
        const int side    = (Value < 50.0f) ? 1 : 2;

        const int LN_INSTANCE_STRIDE = 1000000;
        const int LN_BAR_STRIDE      = 10;

        Tool.LineNumber   = sc.StudyGraphInstanceID * LN_INSTANCE_STRIDE + BarIndex * LN_BAR_STRIDE + side;
        Tool.BeginIndex   = BarIndex;
        Tool.Region       = sc.GraphRegion;
        Tool.BeginValue   = Value + In_TROffset.GetFloat();
        Tool.TextAlignment = DT_CENTER | DT_VCENTER;
        Tool.Color        = RGB(255, 255, 255);
        Tool.FontSize     = In_TRFontSize.GetInt();
        Tool.FontBold     = 1;
        Tool.FontBackColor = RGB(0, 0, 0);
        Tool.TransparentLabelBackground = 1;
        Tool.Text = "TR";
        sc.UseTool(Tool);
    };

    // ===== Helper: draw "B" text in Region 1 =====
    auto DrawB_Region1 = [&](int BarIndex, float Value, COLORREF Color, int Direction /*1=down,2=up*/)
    {
        s_UseTool Tool;
        Tool.Clear();
        Tool.ChartNumber  = sc.ChartNumber;
        Tool.DrawingType  = DRAWING_TEXT;
        Tool.AddMethod    = UTAM_ADD_OR_ADJUST;

        const int LN_INSTANCE_STRIDE = 1000000;
        const int LN_BAR_STRIDE      = 10;

        Tool.LineNumber   = sc.StudyGraphInstanceID * LN_INSTANCE_STRIDE + BarIndex * LN_BAR_STRIDE + 100 + Direction;

        Tool.BeginIndex   = BarIndex;
        Tool.Region       = sc.GraphRegion;
        Tool.BeginValue   = Value + In_BOffset.GetFloat();
        Tool.TextAlignment = DT_CENTER | DT_VCENTER;

        Tool.Color        = Color;
        Tool.FontSize     = In_BFontSize.GetInt();
        Tool.FontBold     = 1;

        Tool.FontBackColor = RGB(0, 0, 0);
        Tool.TransparentLabelBackground = 1;

        Tool.Text = "B";
        sc.UseTool(Tool);
    };

    // ===== Helper: draw divergence labels =====
    auto DrawDivLabel_Region1 = [&](int BarIndex, float Value, const char* Text, COLORREF Color, int Slot)
    {
        s_UseTool Tool;
        Tool.Clear();
        Tool.ChartNumber = sc.ChartNumber;
        Tool.DrawingType = DRAWING_TEXT;
        Tool.AddMethod   = UTAM_ADD_OR_ADJUST;

        const int LN_INSTANCE_STRIDE = 1000000;
        const int LN_BAR_STRIDE      = 10;

        Tool.LineNumber = sc.StudyGraphInstanceID * LN_INSTANCE_STRIDE + BarIndex * LN_BAR_STRIDE + 200 + Slot;
        Tool.BeginIndex = BarIndex;
        Tool.Region     = sc.GraphRegion;
        Tool.BeginValue = Value;
        Tool.TextAlignment = DT_CENTER | DT_VCENTER;

        Tool.Color    = Color;
        Tool.FontSize = In_DivFontSize.GetInt();
        Tool.FontBold = 1;

        Tool.FontBackColor = RGB(0, 0, 0);
        Tool.TransparentLabelBackground = 1;

        Tool.Text = Text;
        sc.UseTool(Tool);
    };

    auto EMA_Update = [&](float prev, float price, int length) -> float
    {
        if (length <= 1) return price;
        const float a = 2.0f / (length + 1.0f);
        return a * price + (1.0f - a) * prev;
    };

    // Clear markers
    SG_SQZ_Buy[i]  = 0.0f;
    SG_SQZ_Sell[i] = 0.0f;

    // ===== RSI (Pine-exact) =====
    const int   Len = In_RsiLength.GetInt();
    const float OB  = In_RsiOB.GetFloat();
    const float OS  = In_RsiOS.GetFloat();

    SG_Midline[i] = 50.0f;

    if (i == 0)
    {
        SG_RSI_Pine[i] = 50.0f;
        SG_RSI_Pine.DataColor[i] = CLR_NEU;

        SG_UpRMA[i] = 0.0f;
        SG_DownRMA[i] = 0.0f;

        // Seed WAE EMA state
        SG_WAE_FastEMA[i] = (float)sc.Close[i];
        SG_WAE_SlowEMA[i] = (float)sc.Close[i];
        SG_WAE_MacdDiff[i] = 0.0f;

        // Seed EMA Trend meter EMA state
        SG_TrendFastEMA[i] = (float)sc.Close[i];
        SG_TrendSlowEMA[i] = (float)sc.Close[i];

        return;
    }

    const float change = (float)(sc.Close[i] - sc.Close[i - 1]);
    const float up     = (change > 0.0f) ? change : 0.0f;
    const float down   = (change < 0.0f) ? -change : 0.0f;

    if (i < Len)
    {
        SG_UpRMA[i] = 0.0f;
        SG_DownRMA[i] = 0.0f;

        SG_RSI_Pine[i] = 50.0f;
        SG_RSI_Pine.DataColor[i] = CLR_NEU;
    }
    else
    {
        if (i == Len)
        {
            float sumUp = 0.0f, sumDown = 0.0f;
            for (int k = 1; k <= Len; ++k)
            {
                const float ch = (float)(sc.Close[k] - sc.Close[k - 1]);
                sumUp   += (ch > 0.0f) ? ch : 0.0f;
                sumDown += (ch < 0.0f) ? -ch : 0.0f;
            }
            SG_UpRMA[i] = sumUp / (float)Len;
            SG_DownRMA[i] = sumDown / (float)Len;
        }
        else
        {
            SG_UpRMA[i] = (SG_UpRMA[i - 1] * (Len - 1) + up) / (float)Len;
            SG_DownRMA[i] = (SG_DownRMA[i - 1] * (Len - 1) + down) / (float)Len;
        }

        const float upRma   = SG_UpRMA[i];
        const float downRma = SG_DownRMA[i];

        float rsiP;
        if      (downRma == 0.0f) rsiP = 100.0f;
        else if (upRma   == 0.0f) rsiP = 0.0f;
        else                      rsiP = 100.0f - (100.0f / (1.0f + (upRma / downRma)));

        rsiP = (rsiP < 0.0f) ? 0.0f : (rsiP > 100.0f) ? 100.0f : rsiP;

        SG_RSI_Pine[i] = rsiP;

        if      (rsiP > OB) SG_RSI_Pine.DataColor[i] = CLR_UP;
        else if (rsiP < OS) SG_RSI_Pine.DataColor[i] = RGB(220, 20, 60);
        else                SG_RSI_Pine.DataColor[i] = CLR_NEU;
    }

    // Engine RSI for Trampoline
    sc.RSI(sc.Close, SG_RSI_Eng, MOVAVGTYPE_SIMPLE, Len);

    // ===== Trampoline (TR text) =====
    if (In_ShowTramp.GetYesNo() && barClosed && i >= 10)
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
            const float b = SG_BB.Arrays[2][idx];
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
                && (BackAt(idx-1) || BackAt(idx-2) || BackAt(idx-3) || BackAt(idx-4) || BackAt(idx-5))
                && sc.High[idx] > sc.High[idx-1];
        };

        auto WeGoDownAt = [&](int idx) -> bool
        {
            if (idx < 10) return false;
            return IsRed(idx)
                && (ForAt(idx-1) || ForAt(idx-2) || ForAt(idx-3) || ForAt(idx-4) || ForAt(idx-5))
                && sc.Low[idx] < sc.Low[idx-1];
        };

        const bool upThrust =
            WeGoUpAt(i) && !WeGoUpAt(i-1) && !WeGoUpAt(i-2) && !WeGoUpAt(i-3) && !WeGoUpAt(i-4);

        const bool downThrust =
            WeGoDownAt(i) && !WeGoDownAt(i-1) && !WeGoDownAt(i-2) && !WeGoDownAt(i-3) && !WeGoDownAt(i-4);

        if (upThrust   && In_ShowTRText.GetYesNo()) DrawTR_Region1(i, 25.0f);
        if (downThrust && In_ShowTRText.GetYesNo()) DrawTR_Region1(i, 75.0f);
    }

    // ===== BB wicks (B text) =====
    if (In_ShowBBWicking.GetYesNo() && barClosed)
    {
        const int   wLen  = In_WickBBLength.GetInt();
        const float wMult = In_WickBBStdDev.GetFloat();

        sc.BollingerBands(sc.Close, SG_WickBB, wLen, wMult, MOVAVGTYPE_SIMPLE);

        const float wUpper = SG_WickBB.Arrays[0][i];
        const float wLower = SG_WickBB.Arrays[1][i];

        const bool wickDown =
            sc.Low[i] <= wLower &&
            sc.Close[i] >= wLower &&
            sc.Close[i] < sc.Open[i];

        const bool wickUp =
            sc.High[i] >= wUpper &&
            sc.Close[i] < wUpper &&
            sc.Close[i] > sc.Open[i];

        if (wickDown) DrawB_Region1(i, 35.0f, RGB(220, 20, 60), 1);
        if (wickUp)   DrawB_Region1(i, 65.0f, CLR_UP, 2);
    }

    // ===== WAE =====
    if (In_ShowWAE.GetYesNo())
    {
        SG_WAE_Line[i] = 50.0f;
        SG_WAE_Line.LineWidth = In_WAEThickness.GetInt();

        if (!barClosed)
        {
            SG_WAE_Line.DataColor[i] = (i > 0) ? SG_WAE_Line.DataColor[i - 1] : CLR_NEU;
        }
        else
        {
            const int fastLen = In_WAEFastLen.GetInt();
            const int slowLen = In_WAESlowLen.GetInt();
            const float sens  = (float)In_WAESensitivity.GetInt();
            const float thr   = (float)In_WAEThreshold.GetInt();

            SG_WAE_FastEMA[i] = EMA_Update(SG_WAE_FastEMA[i - 1], (float)sc.Close[i], fastLen);
            SG_WAE_SlowEMA[i] = EMA_Update(SG_WAE_SlowEMA[i - 1], (float)sc.Close[i], slowLen);
            SG_WAE_MacdDiff[i] = SG_WAE_FastEMA[i] - SG_WAE_SlowEMA[i];

            const float t1 = (SG_WAE_MacdDiff[i] - SG_WAE_MacdDiff[i - 1]) * sens;

            COLORREF c = CLR_NEU;
            if (t1 >= thr) c = CLR_UP;
            else if (-t1 >= thr) c = CLR_DN;

            SG_WAE_Line.DataColor[i] = c;
        }
    }
    else
    {
        SG_WAE_Line[i] = 0.0f;
    }

    // ==========================================================
    // EMA Trend Meter (level 71)
    // ==========================================================
    if (In_ShowTrendMeter.GetYesNo())
    {
        SG_TrendMeter[i] = In_TrendLevel.GetFloat();
        SG_TrendMeter.LineWidth = In_TrendThickness.GetInt();

        if (!barClosed)
        {
            SG_TrendMeter.DataColor[i] = (i > 0) ? SG_TrendMeter.DataColor[i - 1] : CLR_NEU;
        }
        else
        {
            const int fastLen = In_TrendFastLen.GetInt();
            const int slowLen = In_TrendSlowLen.GetInt();

            SG_TrendFastEMA[i] = EMA_Update(SG_TrendFastEMA[i - 1], (float)sc.Close[i], fastLen);
            SG_TrendSlowEMA[i] = EMA_Update(SG_TrendSlowEMA[i - 1], (float)sc.Close[i], slowLen);

            COLORREF c = CLR_NEU;
            if (SG_TrendFastEMA[i] > SG_TrendSlowEMA[i]) c = CLR_UP;
            else if (SG_TrendFastEMA[i] < SG_TrendSlowEMA[i]) c = CLR_DN;

            SG_TrendMeter.DataColor[i] = c;
        }
    }
    else
    {
        SG_TrendMeter[i] = 0.0f;
    }

    // ==========================================================
    // Bull Rush meter (level 30)
    // ==========================================================
    if (In_ShowBullRushMeter.GetYesNo())
    {
        SG_BullRushMeter[i] = In_BullRushLevel.GetFloat();
        SG_BullRushMeter.LineWidth = In_BullRushThickness.GetInt();

        if (!barClosed)
        {
            SG_BullRushMeter.DataColor[i] = (i > 0) ? SG_BullRushMeter.DataColor[i - 1] : CLR_OFF;
        }
        else
        {
            float& brEma9  = sc.GetPersistentFloat(100);
            float& brEma21 = sc.GetPersistentFloat(101);
            float& brEma50 = sc.GetPersistentFloat(102);

            if (i == 1)
            {
                brEma9  = (float)sc.Close[i];
                brEma21 = (float)sc.Close[i];
                brEma50 = (float)sc.Close[i];
            }

            brEma9  = EMA_Update(brEma9,  (float)sc.Close[i], 9);
            brEma21 = EMA_Update(brEma21, (float)sc.Close[i], 21);
            brEma50 = EMA_Update(brEma50, (float)sc.Close[i], 50);

            const bool bullUp =
                (brEma9 > brEma21) &&
                ((float)sc.Close[i] > brEma50) &&
                ((float)sc.Open[i]  > brEma50);

            const bool bullDown =
                (brEma9 < brEma21) &&
                ((float)sc.Close[i] < brEma50) &&
                ((float)sc.Open[i]  < brEma50);

            COLORREF c = CLR_OFF;
            if (bullUp) c = CLR_UP;
            else if (bullDown) c = CLR_DN;

            SG_BullRushMeter.DataColor[i] = c;
        }
    }
    else
    {
        SG_BullRushMeter[i] = 0.0f;
    }

    // ===== Divergence Labels =====
    if (In_ShowDivergence.GetYesNo() && barClosed)
    {
        const int L = In_DivLookbackLeft.GetInt();
        const int R = In_DivLookbackRight.GetInt();
        const int rangeUpper = In_DivRangeUpper.GetInt();
        const int rangeLower = In_DivRangeLower.GetInt();
        const int p = i - R;

        if (p >= L && p >= 0)
        {
            bool isPivotLow = true;
            const float rsi_p = SG_RSI_Pine[p];
            for (int k = p - L; k <= p + R; ++k)
            {
                if (k < 0 || k >= sc.ArraySize) { isPivotLow = false; break; }
                if (k == p) continue;
                if (SG_RSI_Pine[k] <= rsi_p) { isPivotLow = false; break; }
            }

            bool isPivotHigh = true;
            for (int k = p - L; k <= p + R; ++k)
            {
                if (k < 0 || k >= sc.ArraySize) { isPivotHigh = false; break; }
                if (k == p) continue;
                if (SG_RSI_Pine[k] >= rsi_p) { isPivotHigh = false; break; }
            }

            if (isPivotLow)
            {
                const int prevP = (int)SG_DivLastPivotLowIndex[i - 1];
                if (prevP > 0)
                {
                    const int barsSince = p - prevP;
                    const bool inRange = (barsSince >= rangeLower && barsSince <= rangeUpper);
                    const bool rsiHL   = (SG_RSI_Pine[p] > SG_RSI_Pine[prevP]) && inRange;
                    const bool priceLL = (sc.Low[p] < sc.Low[prevP]);

                    if (priceLL && rsiHL)
                        DrawDivLabel_Region1(p, 50.0f, " Bull", CLR_UP, 1);
                }
                SG_DivLastPivotLowIndex[i] = (float)p;
            }
            else
                SG_DivLastPivotLowIndex[i] = SG_DivLastPivotLowIndex[i - 1];

            if (isPivotHigh)
            {
                const int prevP = (int)SG_DivLastPivotHighIndex[i - 1];
                if (prevP > 0)
                {
                    const int barsSince = p - prevP;
                    const bool inRange = (barsSince >= rangeLower && barsSince <= rangeUpper);
                    const bool rsiLH   = (SG_RSI_Pine[p] < SG_RSI_Pine[prevP]) && inRange;
                    const bool priceHH = (sc.High[p] > sc.High[prevP]);

                    if (priceHH && rsiLH)
                        DrawDivLabel_Region1(p, 50.0f, " Bear", CLR_DN, 2);
                }
                SG_DivLastPivotHighIndex[i] = (float)p;
            }
            else
                SG_DivLastPivotHighIndex[i] = SG_DivLastPivotHighIndex[i - 1];
        }
    }

    // ===== Squeeze Relaxer v2.1 =====
    const int   sqLength = 20;
    const int   kcLength = 20;
    const float kcMult   = 1.5f;

    const int   diLen       = In_DiLen.GetInt();
    const int   adxLen      = In_AdxLen.GetInt();
    const int   sqTol       = In_SqTolerance.GetInt();
    const float adxThresh   = (float)In_AdxSqueeze.GetInt();

    float tr_i = 0.0f, plusDM_i = 0.0f, minusDM_i = 0.0f;
    if (i > 0)
    {
        const float h  = (float)sc.High[i];
        const float l  = (float)sc.Low[i];
        const float pc = (float)sc.Close[i - 1];
        const float r1 = h - l;
        const float r2 = fabsf(h - pc);
        const float r3 = fabsf(l - pc);
        tr_i = (r1 >= r2) ? ((r1 >= r3) ? r1 : r3) : ((r2 >= r3) ? r2 : r3);

        const float up5   = (float)sc.High[i]    - (float)sc.High[i - 1];
        const float down5 = (float)sc.Low[i - 1] - (float)sc.Low[i];
        plusDM_i  = (up5   > down5 && up5   > 0.0f) ? up5   : 0.0f;
        minusDM_i = (down5 > up5   && down5 > 0.0f) ? down5 : 0.0f;
    }

    if (i < diLen)
    {
        SG_ADX_trRMA[i]  = 0.0f;
        SG_ADX_pRMA[i]   = 0.0f;
        SG_ADX_mRMA[i]   = 0.0f;
        SG_ADX_adxIn[i]  = 0.0f;
        SG_ADX_adxRMA[i] = 0.0f;
    }
    else if (i == diLen)
    {
        float sumTR = 0.0f, sumP = 0.0f, sumM = 0.0f;
        for (int k = 1; k <= diLen; ++k)
        {
            const float hk  = (float)sc.High[k];
            const float lk  = (float)sc.Low[k];
            const float pck = (float)sc.Close[k - 1];
            const float r1k = hk - lk;
            const float r2k = fabsf(hk - pck);
            const float r3k = fabsf(lk - pck);
            sumTR += (r1k >= r2k) ? ((r1k >= r3k) ? r1k : r3k) : ((r2k >= r3k) ? r2k : r3k);

            const float uk = (float)sc.High[k]    - (float)sc.High[k - 1];
            const float dk = (float)sc.Low[k - 1] - (float)sc.Low[k];
            sumP += (uk > dk && uk > 0.0f) ? uk : 0.0f;
            sumM += (dk > uk && dk > 0.0f) ? dk : 0.0f;
        }

        SG_ADX_trRMA[i] = sumTR / diLen;
        SG_ADX_pRMA[i]  = sumP  / diLen;
        SG_ADX_mRMA[i]  = sumM  / diLen;

        const float trR    = SG_ADX_trRMA[i];
        const float plus_  = (trR != 0.0f) ? (100.0f * SG_ADX_pRMA[i] / trR) : 0.0f;
        const float minus_ = (trR != 0.0f) ? (100.0f * SG_ADX_mRMA[i] / trR) : 0.0f;
        const float sumDI  = plus_ + minus_;
        const float absDI  = fabsf(plus_ - minus_);
        SG_ADX_adxIn[i]  = absDI / (sumDI == 0.0f ? 1.0f : sumDI);
        SG_ADX_adxRMA[i] = 0.0f;
    }
    else
    {
        SG_ADX_trRMA[i] = (SG_ADX_trRMA[i-1] * (diLen - 1) + tr_i)      / (float)diLen;
        SG_ADX_pRMA[i]  = (SG_ADX_pRMA[i-1]  * (diLen - 1) + plusDM_i)  / (float)diLen;
        SG_ADX_mRMA[i]  = (SG_ADX_mRMA[i-1]  * (diLen - 1) + minusDM_i) / (float)diLen;

        const float trR    = SG_ADX_trRMA[i];
        const float plus_  = (trR != 0.0f) ? (100.0f * SG_ADX_pRMA[i] / trR) : 0.0f;
        const float minus_ = (trR != 0.0f) ? (100.0f * SG_ADX_mRMA[i] / trR) : 0.0f;
        const float sumDI  = plus_ + minus_;
        const float absDI  = fabsf(plus_ - minus_);
        SG_ADX_adxIn[i]  = absDI / (sumDI == 0.0f ? 1.0f : sumDI);

        const int seedADX = diLen + adxLen - 1;
        if (i == seedADX)
        {
            float sumADX = 0.0f;
            for (int k = diLen; k <= seedADX; ++k)
                sumADX += SG_ADX_adxIn[k];
            SG_ADX_adxRMA[i] = sumADX / (float)adxLen;
        }
        else if (i > seedADX)
        {
            SG_ADX_adxRMA[i] = (SG_ADX_adxRMA[i-1] * (adxLen - 1) + SG_ADX_adxIn[i]) / (float)adxLen;
        }
        else
        {
            SG_ADX_adxRMA[i] = 0.0f;
        }
    }

    const float adxValue   = (i >= diLen + adxLen - 1) ? (100.0f * SG_ADX_adxRMA[i]) : 0.0f;
    const bool  sigabove19 = adxValue > adxThresh;

    bool sqzOn = false;
    if (i >= sqLength - 1)
    {
        float sumC = 0.0f;
        for (int k = 0; k < sqLength; ++k)
            sumC += (float)sc.Close[i - k];
        const float basis = sumC / (float)sqLength;

        float varC = 0.0f;
        for (int k = 0; k < sqLength; ++k)
        {
            const float d = (float)sc.Close[i - k] - basis;
            varC += d * d;
        }
        const float stdC  = sqrtf(varC / (float)sqLength);
        const float dev1  = kcMult * stdC;
        const float upperBBsq = basis + dev1;
        const float lowerBBsq = basis - dev1;

        float sumCkc = 0.0f, sumRng = 0.0f;
        for (int k = 0; k < kcLength; ++k)
        {
            sumCkc += (float)sc.Close[i - k];
            sumRng += (float)sc.High[i - k] - (float)sc.Low[i - k];
        }
        const float ma_kc   = sumCkc / (float)kcLength;
        const float rngma   = sumRng / (float)kcLength;
        const float upperKC = ma_kc + rngma * kcMult;
        const float lowerKC = ma_kc - rngma * kcMult;

        sqzOn = (lowerBBsq > lowerKC) && (upperBBsq < upperKC);
    }

    if (i >= kcLength - 1)
    {
        float highH = (float)sc.High[i], lowL = (float)sc.Low[i], sumCkc2 = 0.0f;
        for (int k = 0; k < kcLength; ++k)
        {
            if ((float)sc.High[i - k] > highH) highH = (float)sc.High[i - k];
            if ((float)sc.Low[i - k]  < lowL)  lowL  = (float)sc.Low[i - k];
            sumCkc2 += (float)sc.Close[i - k];
        }
        const float avg1  = (highH + lowL) * 0.5f;
        const float smaCl = sumCkc2 / (float)kcLength;
        SG_SQZ_avg2[i] = (avg1 + smaCl) * 0.5f;
    }
    else
    {
        SG_SQZ_avg2[i] = (float)sc.Close[i];
    }

    SG_SQZ_val[i] = 0.0f;
    if (i >= 2 * kcLength - 2)
    {
        const int n = kcLength;
        float sum_x = 0.0f, sum_x2 = 0.0f, sum_y = 0.0f, sum_xy = 0.0f;
        for (int k = 0; k < n; ++k)
        {
            const int   j  = i - (n - 1) + k;
            const float y  = (float)sc.Close[j] - SG_SQZ_avg2[j];
            const float fk = (float)k;
            sum_x  += fk;
            sum_x2 += fk * fk;
            sum_y  += y;
            sum_xy += fk * y;
        }
        const float denom = (float)n * sum_x2 - sum_x * sum_x;
        const float slope = (denom != 0.0f) ? ((float)n * sum_xy - sum_x * sum_y) / denom : 0.0f;
        const float intercept = (sum_y - slope * sum_x) / (float)n;
        SG_SQZ_val[i] = intercept + slope * (float)(n - 1);
    }

    const float val_cur  = SG_SQZ_val[i];
    const float val_prev = (i > 0) ? SG_SQZ_val[i - 1] : 0.0f;

    float cRed_cur   = (i > 0) ? SG_SQZ_cRed[i - 1]   : 0.0f;
    float cGreen_cur = (i > 0) ? SG_SQZ_cGreen[i - 1] : 0.0f;
    const bool pos_prev = (i > 0) && (SG_SQZ_pos[i - 1] != 0.0f);
    const bool neg_prev = (i > 0) && (SG_SQZ_neg[i - 1] != 0.0f);

    bool pos_cur = false;
    bool neg_cur = false;

    const float SQZ_VAL_MID = 5.0f;

    if (i >= 2 * kcLength - 2)
    {
        if (val_cur < val_prev && val_cur < SQZ_VAL_MID && !sqzOn)
            cRed_cur += 1.0f;

        if (val_cur > val_prev && val_cur > SQZ_VAL_MID && !sqzOn)
            cGreen_cur += 1.0f;

        if (val_cur > val_prev && cRed_cur > (float)sqTol && val_cur < SQZ_VAL_MID && !pos_prev && sigabove19)
        {
            cRed_cur = 0.0f;
            pos_cur  = true;
        }

        if (val_cur < val_prev && cGreen_cur > (float)sqTol && val_cur > SQZ_VAL_MID && !neg_prev && sigabove19)
        {
            cGreen_cur = 0.0f;
            neg_cur    = true;
        }
    }

    SG_SQZ_cRed[i]   = cRed_cur;
    SG_SQZ_cGreen[i] = cGreen_cur;
    SG_SQZ_pos[i]    = pos_cur ? 1.0f : 0.0f;
    SG_SQZ_neg[i]    = neg_cur ? 1.0f : 0.0f;

    if (In_ShowSqueeze.GetYesNo() && barClosed)
    {
        if (pos_cur) SG_SQZ_Buy[i]  = 25.0f;
        if (neg_cur) SG_SQZ_Sell[i] = 75.0f;
    }
}