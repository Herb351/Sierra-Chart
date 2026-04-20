#include "sierrachart.h"

SCDLLName("Nebula WAE Candles")

static float StdDev(const SCFloatArrayRef& in, int i, int length)
{
    if (length <= 1 || i < length - 1)
        return 0.0f;

    double sum = 0.0;
    for (int k = i - length + 1; k <= i; ++k)
        sum += in[k];
    const double mean = sum / length;

    double var = 0.0;
    for (int k = i - length + 1; k <= i; ++k)
    {
        const double d = in[k] - mean;
        var += d * d;
    }
    var /= length;
    return (float)sqrt(var);
}

static float Clamp01(float t)
{
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
}

static float Normalize(float value, float minV, float maxV)
{
    if (maxV <= minV)
        return 1.0f;
    return Clamp01((value - minV) / (maxV - minV));
}

static COLORREF LerpColorRGB(float t, COLORREF c1, COLORREF c2)
{
    t = Clamp01(t);
    const int r1 = GetRValue(c1), g1 = GetGValue(c1), b1 = GetBValue(c1);
    const int r2 = GetRValue(c2), g2 = GetGValue(c2), b2 = GetBValue(c2);

    const int r = (int)(r1 + t * (r2 - r1));
    const int g = (int)(g1 + t * (g2 - g1));
    const int b = (int)(b1 + t * (b2 - b1));
    return RGB(r, g, b);
}

SCSFExport scsf_Nebula_WaddahExplosionCandles(SCStudyInterfaceRef sc)
{
    SCSubgraphRef SG_T1    = sc.Subgraph[0];
    SCSubgraphRef SG_Color = sc.Subgraph[1];
    SCSubgraphRef SG_Up    = sc.Subgraph[2];
    SCSubgraphRef SG_Down  = sc.Subgraph[3];

    SCInputRef In_Sensitivity   = sc.Input[0];
    SCInputRef In_FastLength    = sc.Input[1];
    SCInputRef In_SlowLength    = sc.Input[2];
    SCInputRef In_ChannelLength = sc.Input[3];
    SCInputRef In_BBMult        = sc.Input[4];
    SCInputRef In_TopBody       = sc.Input[5];

    // New: customizable gradient endpoints + weak colors
    SCInputRef In_UpStrongLowColor  = sc.Input[6];
    SCInputRef In_UpStrongHighColor = sc.Input[7];
    SCInputRef In_UpWeakColor       = sc.Input[8];

    SCInputRef In_DnStrongLowColor  = sc.Input[9];
    SCInputRef In_DnStrongHighColor = sc.Input[10];
    SCInputRef In_DnWeakColor       = sc.Input[11];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Nebula - Waddah Explosion Candles";
        sc.StudyDescription = "Outputs Waddah Explosion candle colors via Subgraph DataColor. Use Chart Settings -> Color Bar Based On Study.";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;
        sc.DrawZeros = 0;

        SG_T1.Name = "WAE t1 (internal)";
        SG_T1.DrawStyle = DRAWSTYLE_IGNORE;

        SG_Color.Name = "WAE Candle Color";
        SG_Color.DrawStyle = DRAWSTYLE_COLOR_BAR;
        SG_Color.PrimaryColor = RGB(255, 255, 255);
        SG_Color.DrawZeros = 0;

        SG_Up.Name = "TrendUpWAE (internal)";
        SG_Up.DrawStyle = DRAWSTYLE_IGNORE;

        SG_Down.Name = "TrendDownWAE (internal)";
        SG_Down.DrawStyle = DRAWSTYLE_IGNORE;

        In_Sensitivity.Name = "Sensitivity";
        In_Sensitivity.SetInt(150);

        In_FastLength.Name = "Fast EMA Length";
        In_FastLength.SetInt(20);

        In_SlowLength.Name = "Slow EMA Length";
        In_SlowLength.SetInt(40);

        In_ChannelLength.Name = "BB Channel Length";
        In_ChannelLength.SetInt(20);

        In_BBMult.Name = "BB Stdev Multiplier";
        In_BBMult.SetFloat(2.0f);

        In_TopBody.Name = "Top WAE Body Value (gradient max)";
        In_TopBody.SetInt(80);

        // Defaults similar to your earlier approach
        In_UpStrongLowColor.Name = "Up Strong (Low Intensity) Color";
        In_UpStrongLowColor.SetColor(RGB(0, 255, 0));          // you can set darker if you want

        In_UpStrongHighColor.Name = "Up Strong (High Intensity) Color";
        In_UpStrongHighColor.SetColor(RGB(0, 255, 0));

        In_UpWeakColor.Name = "Up Weak Color";
        In_UpWeakColor.SetColor(RGB(40, 80, 40));              // dim green

        In_DnStrongLowColor.Name = "Down Strong (Low Intensity) Color";
        In_DnStrongLowColor.SetColor(RGB(255, 0, 0));

        In_DnStrongHighColor.Name = "Down Strong (High Intensity) Color";
        In_DnStrongHighColor.SetColor(RGB(255, 0, 0));

        In_DnWeakColor.Name = "Down Weak Color";
        In_DnWeakColor.SetColor(RGB(80, 40, 40));              // dim red

        return;
    }

    const int i = sc.Index;
    if (i < 2)
        return;

    const int sens     = In_Sensitivity.GetInt();
    const int fastLen  = In_FastLength.GetInt();
    const int slowLen  = In_SlowLength.GetInt();
    const int chLen    = In_ChannelLength.GetInt();
    const float bbMult = In_BBMult.GetFloat();
    const int topBody  = In_TopBody.GetInt();

    // EMA fast/slow
    SCFloatArrayRef A_fast = SG_T1.Arrays[0];
    SCFloatArrayRef A_slow = SG_T1.Arrays[1];
    sc.ExponentialMovAvg(sc.Close, A_fast, i, fastLen);
    sc.ExponentialMovAvg(sc.Close, A_slow, i, slowLen);

    const float macdDiffNow  = A_fast[i] - A_slow[i];
    const float macdDiffPrev = A_fast[i - 1] - A_slow[i - 1];

    const float t1 = (macdDiffNow - macdDiffPrev) * (float)sens;

    const float st = StdDev(sc.Close, i, chLen);
    const float e1 = 2.0f * (bbMult * st);

    const float trendUpWAE   = (t1 >= 0.0f) ? t1 : 0.0f;
    const float trendDownWAE = (t1 < 0.0f) ? (-t1) : 0.0f;

    SG_T1[i] = t1;
    SG_Up[i] = trendUpWAE;
    SG_Down[i] = trendDownWAE;

    const COLORREF upLow  = In_UpStrongLowColor.GetColor();
    const COLORREF upHigh = In_UpStrongHighColor.GetColor();
    const COLORREF upWeak = In_UpWeakColor.GetColor();

    const COLORREF dnLow  = In_DnStrongLowColor.GetColor();
    const COLORREF dnHigh = In_DnStrongHighColor.GetColor();
    const COLORREF dnWeak = In_DnWeakColor.GetColor();

    COLORREF bodyColor = RGB(255, 255, 255);

    if (trendUpWAE > 0.0f)
    {
        if (trendUpWAE > e1)
        {
            // Strength is abs(trendUpWAE - e1), Pine uses [1..TopBody]
            const float v = fabsf(trendUpWAE - e1);
            const float t = Normalize(v, 1.0f, (float)topBody);
            bodyColor = LerpColorRGB(t, upLow, upHigh);
        }
        else
        {
            bodyColor = upWeak;
        }
    }
    else if (trendDownWAE > 0.0f)
    {
        if (trendDownWAE > e1)
        {
            const float v = fabsf(trendDownWAE - e1);
            const float t = Normalize(v, 1.0f, (float)topBody);
            bodyColor = LerpColorRGB(t, dnLow, dnHigh);
        }
        else
        {
            bodyColor = dnWeak;
        }
    }
    else
    {
        // No signal -> set to white (or you can set IGNORE by not setting DataColor, but keep simple)
        bodyColor = RGB(255, 255, 255);
    }

    SG_Color[i] = 1.0f;
    SG_Color.DataColor[i] = bodyColor;
}