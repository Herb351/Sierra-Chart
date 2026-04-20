// Sierra Chart ACSIL - Standalone Study
// Nebula - Cloud [Simple only; older fill]
// Includes configurable cloud up/down colors (Settings and Inputs tab).


#include "sierrachart.h"

SCDLLName("Nebula Cloud")

// ===================== Helpers =====================

static float SMA(const SCFloatArrayRef& in, int i, int length)
{
    if (length <= 0 || i < length - 1) return 0.0f;
    double sum = 0.0;
    for (int k = i - length + 1; k <= i; ++k) sum += in[k];
    return (float)(sum / length);
}

// ===================== Study =====================

SCSFExport scsf_Nebula_Cloud(SCStudyInterfaceRef sc)
{
    SCSubgraphRef SG_MG = sc.Subgraph[0];
    SCSubgraphRef SG_FanVMA = sc.Subgraph[1];
    SCSubgraphRef SG_CloudTop = sc.Subgraph[2];
    SCSubgraphRef SG_CloudBot = sc.Subgraph[3];

    SCInputRef In_ADX_Length = sc.Input[0];
    SCInputRef In_Weighting = sc.Input[1];
    SCInputRef In_MA_Length = sc.Input[2];
    SCInputRef In_McGinleyLen = sc.Input[3];
    SCInputRef In_CloudDarkness = sc.Input[4];

    // Cloud fill colors (editable on Settings and Inputs tab)
    SCInputRef In_CloudUpColor = sc.Input[5];
    SCInputRef In_CloudDownColor = sc.Input[6];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Nebula - Cloud";
        sc.StudyDescription = "Nebula Simple Cloud: FanVMA vs McGinley Dynamic with filled cloud (Simple only).";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;
        sc.DrawZeros = 0;

        SG_MG.Name = "McGinley Dynamic";
        SG_MG.DrawStyle = DRAWSTYLE_LINE;
        SG_MG.PrimaryColor = RGB(80, 80, 255);
        SG_MG.LineWidth = 2;

        SG_FanVMA.Name = "Fantail VMA";
        SG_FanVMA.DrawStyle = DRAWSTYLE_LINE;
        SG_FanVMA.PrimaryColor = RGB(200, 200, 200);
        SG_FanVMA.LineWidth = 2;

        SG_CloudTop.Name = "Cloud Top";
        SG_CloudTop.DrawStyle = DRAWSTYLE_FILL_TOP;
        SG_CloudTop.PrimaryColor = RGB(0, 255, 0);

        SG_CloudBot.Name = "Cloud Bottom";
        SG_CloudBot.DrawStyle = DRAWSTYLE_FILL_BOTTOM;
        SG_CloudBot.PrimaryColor = RGB(0, 255, 0);

        In_ADX_Length.Name = "FantailVMA ADX_Length";
        In_ADX_Length.SetInt(2);

        In_Weighting.Name = "FantailVMA Weighting";
        In_Weighting.SetFloat(10.0f);

        In_MA_Length.Name = "FantailVMA MA_Length";
        In_MA_Length.SetInt(6);

        In_McGinleyLen.Name = "McGinley Length";
        In_McGinleyLen.SetInt(14);

        In_CloudDarkness.Name = "Cloud Darkness (0 bright, 100 dark)";
        In_CloudDarkness.SetInt(30);
        In_CloudDarkness.SetIntLimits(0, 100);

        In_CloudUpColor.Name = "Cloud Up Color";
        In_CloudUpColor.SetColor(RGB(0, 255, 0));

        In_CloudDownColor.Name = "Cloud Down Color";
        In_CloudDownColor.SetColor(RGB(255, 0, 0));

        return;
    }

    const int i = sc.Index;
    if (i < 2)
        return;

    const int adxLen = In_ADX_Length.GetInt();
    const float Weighting = In_Weighting.GetFloat();
    const int maLen = In_MA_Length.GetInt();
    const int mcLen = In_McGinleyLen.GetInt();
    const int darkness = In_CloudDarkness.GetInt();

    const COLORREF upBaseColor = In_CloudUpColor.GetColor();
    const COLORREF downBaseColor = In_CloudDownColor.GetColor();

    SCFloatArrayRef sPDI = SG_FanVMA.Arrays[0];
    SCFloatArrayRef sMDI = SG_FanVMA.Arrays[1];
    SCFloatArrayRef STR = SG_FanVMA.Arrays[2];
    SCFloatArrayRef ADX = SG_FanVMA.Arrays[3];
    SCFloatArrayRef VarMA = SG_FanVMA.Arrays[4];

    const float Hi = sc.High[i];
    const float Lo = sc.Low[i];
    const float Hi1 = sc.High[i - 1];
    const float Lo1 = sc.Low[i - 1];
    const float Close1 = sc.Close[i - 1];

    const float Bulls1 = 0.5f * (fabsf(Hi - Hi1) + (Hi - Hi1));
    const float Bears1 = 0.5f * (fabsf(Lo1 - Lo) + (Lo1 - Lo));

    const float Bears = (Bulls1 > Bears1) ? 0.0f : ((Bulls1 == Bears1) ? 0.0f : Bears1);
    const float Bulls = (Bulls1 < Bears1) ? 0.0f : ((Bulls1 == Bears1) ? 0.0f : Bulls1);

    if (i == 1)
    {
        sPDI[i] = 0.0f;
        sMDI[i] = 0.0f;
        STR[i] = Hi - Lo;
        ADX[i] = 0.0f;
        VarMA[i] = sc.Close[i];
    }
    else
    {
        sPDI[i] = (Weighting * sPDI[i - 1] + Bulls) / (Weighting + 1.0f);
        sMDI[i] = (Weighting * sMDI[i - 1] + Bears) / (Weighting + 1.0f);

        const float TR = fmaxf(Hi - Lo, fmaxf(fabsf(Hi - Close1), fabsf(Lo - Close1)));
        STR[i] = (Weighting * STR[i - 1] + TR) / (Weighting + 1.0f);

        const float PDI = (STR[i] > 0.0f) ? (sPDI[i] / STR[i]) : 0.0f;
        const float MDI = (STR[i] > 0.0f) ? (sMDI[i] / STR[i]) : 0.0f;
        const float DX = ((PDI + MDI) > 0.0f) ? (fabsf(PDI - MDI) / (PDI + MDI)) : 0.0f;

        ADX[i] = (Weighting * ADX[i - 1] + DX) / (Weighting + 1.0f);

        float adxMin = ADX[i];
        float adxMax = ADX[i];
        for (int k = 0; k < adxLen && i - k >= 0; ++k)
        {
            adxMin = fminf(adxMin, ADX[i - k]);
            adxMax = fmaxf(adxMax, ADX[i - k]);
        }
        const float Diff = adxMax - adxMin;
        const float Const = (Diff > 0.0f) ? ((ADX[i] - adxMin) / Diff) : 0.0f;

        VarMA[i] = ((2.0f - Const) * VarMA[i - 1] + Const * sc.Close[i]) / 2.0f;
    }

    SG_FanVMA[i] = SMA(VarMA, i, maLen);

    if (i == 1 || SG_MG[i - 1] == 0.0f)
    {
        SCFloatArrayRef tmp = SG_MG.Arrays[0];
        sc.ExponentialMovAvg(sc.Close, tmp, i, mcLen);
        SG_MG[i] = tmp[i];
    }
    else
    {
        const float mgPrev = SG_MG[i - 1];
        const float closeNow = sc.Close[i];
        float ratioPow = 1.0f;
        if (mgPrev != 0.0f)
        {
            const float ratio = closeNow / mgPrev;
            ratioPow = powf(ratio, 4.0f);
            if (ratioPow == 0.0f)
                ratioPow = 1.0f;
        }
        SG_MG[i] = mgPrev + (closeNow - mgPrev) / ((float)mcLen * ratioPow);
    }

    const bool cloudUp = (SG_FanVMA[i] > SG_MG[i]);

    auto Darken = [&](COLORREF c) -> COLORREF
    {
        const float t = (100.0f - (float)darkness) / 100.0f;
        return RGB((int)(GetRValue(c) * t), (int)(GetGValue(c) * t), (int)(GetBValue(c) * t));
    };

    const COLORREF fillColor = cloudUp ? Darken(upBaseColor) : Darken(downBaseColor);

    const float top = fmaxf(SG_FanVMA[i], SG_MG[i]);
    const float bot = fminf(SG_FanVMA[i], SG_MG[i]);

    SG_CloudTop[i] = top;
    SG_CloudBot[i] = bot;

    // Per-bar coloring required for auto-switching up/down colors
    SG_CloudTop.DataColor[i] = fillColor;
    SG_CloudBot.DataColor[i] = fillColor;
}