#include "sierrachart.h"

SCDLLName("Nebula Ultimate Buy/Sell Signals")

static const int UP_TREND = 1;
static const int DOWN_TREND = 2;

static float RMA_Update(float prev, float x, int length)
{
    const float a = 1.0f / (float)length; // Pine ta.rma alpha
    return prev + a * (x - prev);
}

static float SMA(const SCFloatArrayRef& a, int index, int length)
{
    const int start = max(0, index - length + 1);
    const int count = index - start + 1;
    if (count <= 0) return 0.0f;

    double sum = 0.0;
    for (int i = start; i <= index; ++i) sum += a[i];
    return (float)(sum / (double)count);
}

static float StdDev(const SCFloatArrayRef& a, int index, int length)
{
    const int start = max(0, index - length + 1);
    const int count = index - start + 1;
    if (count <= 1) return 0.0f;

    const float mean = SMA(a, index, length);

    double var = 0.0;
    for (int i = start; i <= index; ++i)
    {
        const double d = (double)a[i] - (double)mean;
        var += d * d;
    }
    var /= (double)count;

    return (float)sqrt(var);
}

static float WMA(const SCFloatArrayRef& a, int index, int length)
{
    const int start = max(0, index - length + 1);
    const int count = index - start + 1;
    if (count <= 0) return 0.0f;

    const int L = min(length, count);

    double sumW = 0.0;
    double sum = 0.0;
    int w = 1;
    for (int i = index - L + 1; i <= index; ++i, ++w)
    {
        sum += (double)a[i] * (double)w;
        sumW += (double)w;
    }
    return (float)(sum / sumW);
}

SCSFExport scsf_Nebula_Ultimate_Buy_Sell_Signals(SCStudyInterfaceRef sc)
{
    // Outputs (overlay)
    SCSubgraphRef BuyWeak    = sc.Subgraph[0];
    SCSubgraphRef SellWeak   = sc.Subgraph[1];
    SCSubgraphRef BuyStrong  = sc.Subgraph[2];
    SCSubgraphRef SellStrong = sc.Subgraph[3];

    // Internal / scratch (hidden)
    SCSubgraphRef RSI_SG       = sc.Subgraph[4];
    SCSubgraphRef RSIBasis_SG  = sc.Subgraph[5];
    SCSubgraphRef UpperRSI_SG  = sc.Subgraph[6];
    SCSubgraphRef LowerRSI_SG  = sc.Subgraph[7];

    SCSubgraphRef PriceBasis_SG = sc.Subgraph[8];
    SCSubgraphRef UpperInner_SG = sc.Subgraph[9];
    SCSubgraphRef LowerInner_SG = sc.Subgraph[10];

    SCSubgraphRef ATR_SG          = sc.Subgraph[11];
    SCSubgraphRef ATRMA_SG        = sc.Subgraph[12];
    SCSubgraphRef UpperAtrBand_SG = sc.Subgraph[13];
    SCSubgraphRef LowerAtrBand_SG = sc.Subgraph[14];

    // Inputs
    SCInputRef DrawOffsetTicks = sc.Input[0];
    SCInputRef WeakPointSize   = sc.Input[1];
    SCInputRef StrongPointSize = sc.Input[2];

    SCInputRef WatchLookback   = sc.Input[10];
    SCInputRef RequireWatch    = sc.Input[11];

    SCInputRef RsiLength       = sc.Input[20];
    SCInputRef RsiBasisLength  = sc.Input[21];
    SCInputRef RsiMult         = sc.Input[22];
    SCInputRef RsiMaLen        = sc.Input[23];

    SCInputRef UseRsiWatch     = sc.Input[30];
    SCInputRef UsePriceWatch   = sc.Input[31];
    SCInputRef UseAtrWatch     = sc.Input[32];

    SCInputRef UseRsiSignals   = sc.Input[40];
    SCInputRef Use25Signals    = sc.Input[41];
    SCInputRef Use75Signals    = sc.Input[42];
    SCInputRef UseRsiMaSignals = sc.Input[43];

    SCInputRef PriceBasisLen   = sc.Input[50];
    SCInputRef PriceInnerMult  = sc.Input[51];
    SCInputRef PriceOuterMult  = sc.Input[52]; // kept for parity (not used directly)
    SCInputRef AtrPeriod       = sc.Input[53];
    SCInputRef AtrMaPeriod     = sc.Input[54];
    SCInputRef AtrMult         = sc.Input[55];

    SCInputRef IgnoreDojis      = sc.Input[70];
    SCInputRef MaxDojiBodyTicks = sc.Input[71];
    SCInputRef BackscanBars     = sc.Input[72];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Nebula Signals (Ultimate Buy/Sell full port + Tidal Wave) - Offset Markers";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;

        // IMPORTANT: Use DRAWSTYLE_POINT so the Y-value (High+offset / Low-offset) is honored.
        BuyWeak.Name = "BUY (weak)";
        BuyWeak.DrawStyle = DRAWSTYLE_POINT;
        BuyWeak.PrimaryColor = RGB(0, 255, 132);
        BuyWeak.LineWidth = 3;
        BuyWeak.DrawZeros = false;

        SellWeak.Name = "SELL (weak)";
        SellWeak.DrawStyle = DRAWSTYLE_POINT;
        SellWeak.PrimaryColor = RGB(255, 0, 0);
        SellWeak.LineWidth = 3;
        SellWeak.DrawZeros = false;

        BuyStrong.Name = "BUY (strong)";
        BuyStrong.DrawStyle = DRAWSTYLE_POINT;
        BuyStrong.PrimaryColor = RGB(0, 255, 132);
        BuyStrong.LineWidth = 5;
        BuyStrong.DrawZeros = false;

        SellStrong.Name = "SELL (strong)";
        SellStrong.DrawStyle = DRAWSTYLE_POINT;
        SellStrong.PrimaryColor = RGB(255, 0, 0);
        SellStrong.LineWidth = 5;
        SellStrong.DrawZeros = false;

        // Hidden internals
        RSI_SG.Name = "RSI"; RSI_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        RSIBasis_SG.Name = "RSI Basis"; RSIBasis_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        UpperRSI_SG.Name = "Upper RSI"; UpperRSI_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        LowerRSI_SG.Name = "Lower RSI"; LowerRSI_SG.DrawStyle = DRAWSTYLE_HIDDEN;

        PriceBasis_SG.Name = "Price Basis"; PriceBasis_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        UpperInner_SG.Name = "Upper Inner"; UpperInner_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        LowerInner_SG.Name = "Lower Inner"; LowerInner_SG.DrawStyle = DRAWSTYLE_HIDDEN;

        ATR_SG.Name = "ATR"; ATR_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        ATRMA_SG.Name = "ATRMA"; ATRMA_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        UpperAtrBand_SG.Name = "Upper ATR Band"; UpperAtrBand_SG.DrawStyle = DRAWSTYLE_HIDDEN;
        LowerAtrBand_SG.Name = "Lower ATR Band"; LowerAtrBand_SG.DrawStyle = DRAWSTYLE_HIDDEN;

        DrawOffsetTicks.Name = "Vertical offset (ticks)";
        DrawOffsetTicks.SetInt(4);
        DrawOffsetTicks.SetIntLimits(0, 200);

        WeakPointSize.Name = "Weak point size";
        WeakPointSize.SetInt(3);
        WeakPointSize.SetIntLimits(1, 20);

        StrongPointSize.Name = "Strong point size";
        StrongPointSize.SetInt(5);
        StrongPointSize.SetIntLimits(1, 20);

        WatchLookback.Name = "Watch signal lookback (bars)";
        WatchLookback.SetInt(35);
        WatchLookback.SetIntLimits(1, 500);

        RequireWatch.Name = "Require watch signals";
        RequireWatch.SetYesNo(1);

        RsiLength.Name = "RSI Length";
        RsiLength.SetInt(32);
        RsiLength.SetIntLimits(1, 200);

        RsiBasisLength.Name = "RSI Basis Length (WMA)";
        RsiBasisLength.SetInt(32);
        RsiBasisLength.SetIntLimits(1, 500);

        RsiMult.Name = "RSI Band Multiplier";
        RsiMult.SetFloat(2.0f);
        RsiMult.SetFloatLimits(0.1f, 10.0f);

        RsiMaLen.Name = "Extra RSI MA Length (WMA)";
        RsiMaLen.SetInt(24);
        RsiMaLen.SetIntLimits(1, 500);

        UseRsiWatch.Name = "Use RSI watch signals";
        UseRsiWatch.SetYesNo(1);

        UsePriceWatch.Name = "Use price BB watch signals";
        UsePriceWatch.SetYesNo(1);

        UseAtrWatch.Name = "Use ATR watch signals";
        UseAtrWatch.SetYesNo(1);

        UseRsiSignals.Name = "Use RSI cross basis";
        UseRsiSignals.SetYesNo(1);

        Use25Signals.Name = "Use RSI cross over 25 (buy)";
        Use25Signals.SetYesNo(1);

        Use75Signals.Name = "Use RSI cross under 75 (sell)";
        Use75Signals.SetYesNo(1);

        UseRsiMaSignals.Name = "Use RSI cross extra MA";
        UseRsiMaSignals.SetYesNo(1);

        PriceBasisLen.Name = "Price BB basis length (SMA)";
        PriceBasisLen.SetInt(20);
        PriceBasisLen.SetIntLimits(1, 500);

        PriceInnerMult.Name = "Price BB inner multiplier";
        PriceInnerMult.SetFloat(2.0f);
        PriceInnerMult.SetFloatLimits(0.1f, 20.0f);

        PriceOuterMult.Name = "Price BB outer multiplier (kept for parity)";
        PriceOuterMult.SetFloat(2.5f);
        PriceOuterMult.SetFloatLimits(0.1f, 20.0f);

        AtrPeriod.Name = "ATR Period (Wilder)";
        AtrPeriod.SetInt(30);
        AtrPeriod.SetIntLimits(1, 500);

        AtrMaPeriod.Name = "ATR MA Period (WMA of Close)";
        AtrMaPeriod.SetInt(10);
        AtrMaPeriod.SetIntLimits(1, 500);

        AtrMult.Name = "ATR Multiplier";
        AtrMult.SetFloat(1.5f);
        AtrMult.SetFloatLimits(0.1f, 10.0f);

        IgnoreDojis.Name = "Ignore dojis (Tidal Wave)";
        IgnoreDojis.SetYesNo(0);

        MaxDojiBodyTicks.Name = "Doji body threshold (ticks)";
        MaxDojiBodyTicks.SetInt(1);
        MaxDojiBodyTicks.SetIntLimits(1, 10);

        BackscanBars.Name = "Tidal Wave backscan bars";
        BackscanBars.SetInt(200);
        BackscanBars.SetIntLimits(10, 500);

        return;
    }

    const int i = sc.Index;

    // point sizes (LineWidth controls dot size)
    BuyWeak.LineWidth    = WeakPointSize.GetInt();
    SellWeak.LineWidth   = WeakPointSize.GetInt();
    BuyStrong.LineWidth  = StrongPointSize.GetInt();
    SellStrong.LineWidth = StrongPointSize.GetInt();

    const float tick = (float)sc.TickSize;
    const float yOff = (float)DrawOffsetTicks.GetInt() * tick;

    auto CrossOver = [&](float a0, float a1, float b0, float b1) -> bool
    {
        return (a1 <= b1) && (a0 > b0);
    };
    auto CrossUnder = [&](float a0, float a1, float b0, float b1) -> bool
    {
        return (a1 >= b1) && (a0 < b0);
    };

    // "confirmed" behavior: historical bars are closed; treat last bar as confirmed only if chart says so
    const bool confirmed =
        (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_CLOSED) || (i < sc.ArraySize - 1);

    // ============================
    // Ultimate Buy/Sell (ported core)
    // ============================
    const int rsiLen = RsiLength.GetInt();
    const int rsiBasisLen = RsiBasisLength.GetInt();
    const float rsiMult = RsiMult.GetFloat();
    const int rsiMaLen = RsiMaLen.GetInt();

    float change = (i > 0) ? (sc.Close[i] - sc.Close[i - 1]) : 0.0f;
    float upMove = max(change, 0.0f);
    float dnMove = max(-change, 0.0f);

    float upRMA_prev = (i > 0) ? sc.GetPersistentFloat(10000 + (i - 1)) : 0.0f;
    float dnRMA_prev = (i > 0) ? sc.GetPersistentFloat(11000 + (i - 1)) : 0.0f;

    float upRMA = (i == 0) ? upMove : RMA_Update(upRMA_prev, upMove, rsiLen);
    float dnRMA = (i == 0) ? dnMove : RMA_Update(dnRMA_prev, dnMove, rsiLen);

    sc.SetPersistentFloat(10000 + i, upRMA);
    sc.SetPersistentFloat(11000 + i, dnRMA);

    float rsi = (dnRMA == 0.0f) ? 100.0f : (100.0f - (100.0f / (1.0f + (upRMA / dnRMA))));
    RSI_SG[i] = rsi;

    RSIBasis_SG[i] = WMA(RSI_SG, i, rsiBasisLen);

    const float devRSI = StdDev(RSI_SG, i, rsiBasisLen);
    UpperRSI_SG[i] = RSIBasis_SG[i] + rsiMult * devRSI;
    LowerRSI_SG[i] = RSIBasis_SG[i] - rsiMult * devRSI;

    const float rsiExtraMA = WMA(RSI_SG, i, rsiMaLen);
    const float rsiExtraMA_prev = (i > 0) ? WMA(RSI_SG, i - 1, rsiMaLen) : rsiExtraMA;

    // Price BB (basis SMA, inner bands)
    const int priceBasisLen = PriceBasisLen.GetInt();
    const float priceInnerMult = PriceInnerMult.GetFloat();

    PriceBasis_SG[i] = SMA(sc.Close, i, priceBasisLen);
    const float devPrice = StdDev(sc.Close, i, priceBasisLen);

    UpperInner_SG[i] = PriceBasis_SG[i] + priceInnerMult * devPrice;
    LowerInner_SG[i] = PriceBasis_SG[i] - priceInnerMult * devPrice;

    // ATR + bands
    const int atrPeriod = AtrPeriod.GetInt();
    const int atrMaPeriod = AtrMaPeriod.GetInt();
    const float atrMult = AtrMult.GetFloat();

    sc.ATR(sc.BaseDataIn, ATR_SG, atrPeriod, MOVAVGTYPE_WILDERS, i);
    ATRMA_SG[i] = WMA(sc.Close, i, atrMaPeriod);

    UpperAtrBand_SG[i] = ATRMA_SG[i] + ATR_SG[i] * atrMult;
    LowerAtrBand_SG[i] = ATRMA_SG[i] - ATR_SG[i] * atrMult;

    bool priceCrossOverInner = false, priceCrossUnderInner = false;
    bool rsiCrossOverLower = false, rsiCrossUnderUpper = false;
    bool rsiCrossOverBasis = false, rsiCrossUnderBasis = false;
    bool rsiCrossOverMa = false, rsiCrossUnderMa = false;
    bool rsiCrossUnder75 = false, rsiCrossOver25 = false;
    bool highUnderAtrLower = false, lowOverAtrUpper = false;

    if (i > 0)
    {
        priceCrossOverInner  = CrossOver(sc.Close[i], sc.Close[i - 1], LowerInner_SG[i], LowerInner_SG[i - 1]);
        priceCrossUnderInner = CrossUnder(sc.Close[i], sc.Close[i - 1], UpperInner_SG[i], UpperInner_SG[i - 1]);

        rsiCrossOverLower = CrossOver(RSI_SG[i], RSI_SG[i - 1], LowerRSI_SG[i], LowerRSI_SG[i - 1]);
        rsiCrossUnderUpper= CrossUnder(RSI_SG[i], RSI_SG[i - 1], UpperRSI_SG[i], UpperRSI_SG[i - 1]);

        rsiCrossOverBasis = CrossOver(RSI_SG[i], RSI_SG[i - 1], RSIBasis_SG[i], RSIBasis_SG[i - 1]);
        rsiCrossUnderBasis= CrossUnder(RSI_SG[i], RSI_SG[i - 1], RSIBasis_SG[i], RSIBasis_SG[i - 1]);

        rsiCrossOverMa = CrossOver(RSI_SG[i], RSI_SG[i - 1], rsiExtraMA, rsiExtraMA_prev);
        rsiCrossUnderMa= CrossUnder(RSI_SG[i], RSI_SG[i - 1], rsiExtraMA, rsiExtraMA_prev);

        rsiCrossUnder75 = CrossUnder(RSI_SG[i], RSI_SG[i - 1], 75.0f, 75.0f);
        rsiCrossOver25  = CrossOver(RSI_SG[i], RSI_SG[i - 1], 25.0f, 25.0f);

        highUnderAtrLower = CrossUnder(sc.High[i], sc.High[i - 1], LowerAtrBand_SG[i], LowerAtrBand_SG[i - 1]);
        lowOverAtrUpper   = CrossOver(sc.Low[i], sc.Low[i - 1], UpperAtrBand_SG[i], UpperAtrBand_SG[i - 1]);
    }

    const bool signalsBlocked = false;          // matches your Pine snippet behavior
    const bool watchesInsideYellowRsi = false;  // matches your Pine snippet behavior
    const bool buyAndSellInsideYellowRsi = false;

    const bool usePriceWatch = UsePriceWatch.GetYesNo() != 0;
    const bool useRsiWatch   = UseRsiWatch.GetYesNo() != 0;
    const bool useAtrWatch   = UseAtrWatch.GetYesNo() != 0;

    bool buyWatched =
        ((usePriceWatch && priceCrossOverInner && !rsiCrossOverLower) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useRsiWatch   && rsiCrossOverLower  && !priceCrossOverInner) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((usePriceWatch && priceCrossOverInner &&  rsiCrossOverLower) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((usePriceWatch && priceCrossOverInner) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useRsiWatch   && rsiCrossOverLower) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useRsiWatch   && rsiCrossOver25) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useAtrWatch   && highUnderAtrLower) && !watchesInsideYellowRsi && !signalsBlocked);

    bool sellWatched =
        ((usePriceWatch && priceCrossUnderInner && !rsiCrossUnderUpper) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useRsiWatch   && rsiCrossUnderUpper  && !priceCrossUnderInner) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((usePriceWatch && priceCrossUnderInner &&  rsiCrossUnderUpper) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((usePriceWatch && priceCrossUnderInner) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useRsiWatch   && rsiCrossUnderUpper) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useRsiWatch   && rsiCrossUnder75) && !watchesInsideYellowRsi && !signalsBlocked) ||
        ((useAtrWatch   && lowOverAtrUpper) && !watchesInsideYellowRsi && !signalsBlocked);

    // store watch flags
    sc.SetPersistentInt(12000 + i, buyWatched ? 1 : 0);
    sc.SetPersistentInt(13000 + i, sellWatched ? 1 : 0);

    // emulate Pine "array.clear" by reset pointers
    int lastBuyReset = sc.GetPersistentInt(14000);
    int lastSellReset = sc.GetPersistentInt(14001);

    const int watchLb = WatchLookback.GetInt();

    auto WatchMetSinceReset = [&](bool isBuy) -> bool
    {
        const int lastReset = isBuy ? lastBuyReset : lastSellReset;
        const int start = max(lastReset + 1, i - watchLb + 1);
        for (int idx = start; idx <= i; ++idx)
        {
            if (idx < 0) continue;
            const int v = sc.GetPersistentInt((isBuy ? 12000 : 13000) + idx);
            if (v == 1) return true;
        }
        return false;
    };

    const bool buyWatchMet  = WatchMetSinceReset(true);
    const bool sellWatchMet = WatchMetSinceReset(false);

    bool combinedBuySignals = false;
    bool combinedSellSignals = false;

    if (UseRsiSignals.GetYesNo()) {
        combinedBuySignals  = combinedBuySignals  || rsiCrossOverBasis;
        combinedSellSignals = combinedSellSignals || rsiCrossUnderBasis;
    }
    if (Use25Signals.GetYesNo()) {
        combinedBuySignals  = combinedBuySignals  || rsiCrossOver25;
    }
    if (Use75Signals.GetYesNo()) {
        combinedSellSignals = combinedSellSignals || rsiCrossUnder75;
    }
    if (UseRsiMaSignals.GetYesNo()) {
        combinedBuySignals  = combinedBuySignals  || rsiCrossOverMa;
        combinedSellSignals = combinedSellSignals || rsiCrossUnderMa;
    }

    const bool requireWatch = RequireWatch.GetYesNo() != 0;

    const bool buySignals  = (!requireWatch && combinedBuySignals)  || (requireWatch && buyWatchMet  && combinedBuySignals);
    const bool sellSignals = (!requireWatch && combinedSellSignals) || (requireWatch && sellWatchMet && combinedSellSignals);

    bool plotBuy = false;
    bool plotSell = false;

    if (confirmed && buySignals && !buyAndSellInsideYellowRsi && !buyWatched && !signalsBlocked)
    {
        plotBuy = true;
        lastBuyReset = i;
        lastSellReset = i;
        sc.SetPersistentInt(14000, lastBuyReset);
        sc.SetPersistentInt(14001, lastSellReset);
    }
    else if (confirmed && sellSignals && !buyAndSellInsideYellowRsi && !sellWatched && !signalsBlocked)
    {
        plotSell = true;
        lastBuyReset = i;
        lastSellReset = i;
        sc.SetPersistentInt(14000, lastBuyReset);
        sc.SetPersistentInt(14001, lastSellReset);
    }

    sc.SetPersistentInt(15000 + i, plotBuy ? 1 : 0);
    sc.SetPersistentInt(16000 + i, plotSell ? 1 : 0);

    bool bBigBuy1 = false;
    bool bBigSell1 = false;
    for (int k = 0; k <= 3; ++k)
    {
        const int idx = i - k;
        if (idx < 0) break;
        if (sc.GetPersistentInt(15000 + idx) == 1) bBigBuy1 = true;
        if (sc.GetPersistentInt(16000 + idx) == 1) bBigSell1 = true;
    }

    // ============================
    // Tidal Wave (to produce buyChar/sellChar)
    // ============================
    const bool redCandle = sc.Close[i] < sc.Open[i];
    const bool greenCandle = sc.Close[i] > sc.Open[i];

    const float body = fabs(sc.Close[i] - sc.Open[i]);
    const float maxDojiBody = (float)MaxDojiBodyTicks.GetInt() * tick;
    const bool bDoji = (body <= maxDojiBody) && (IgnoreDojis.GetYesNo() != 0);

    int waveStatePrev = (i > 0) ? sc.GetPersistentInt(17000 + (i - 1)) : 0;
    int waveState = waveStatePrev;

    auto BrightGreenAt = [&](int idx) -> bool { return sc.GetPersistentInt(18000 + idx) == 1; };
    auto BrightRedAt   = [&](int idx) -> bool { return sc.GetPersistentInt(19000 + idx) == 1; };

    const int backscan = BackscanBars.GetInt();
    bool brightGreen = false, brightRed = false;
    bool noOverlapGreen = false, noOverlapRed = false;

    if (greenCandle && !bDoji && confirmed)
    {
        for (int k = 1; k <= backscan; ++k)
        {
            const int idx = i - k;
            if (idx < 0) break;

            if (BrightRedAt(idx)) break;

            const bool redAt = sc.Close[idx] < sc.Open[idx];
            if (waveState == UP_TREND && redAt) break;

            const bool greenAt = sc.Close[idx] > sc.Open[idx];
            if (waveState == DOWN_TREND && sc.Open[i] >= sc.Close[idx] && greenAt)
            {
                noOverlapGreen = true;
                brightGreen = true;
                waveState = UP_TREND;
                break;
            }
        }

        if (i > 0 && sc.Open[i] >= sc.Close[i - 1] && (sc.Close[i - 1] > sc.Open[i - 1]))
        {
            waveState = UP_TREND;
            brightGreen = true;
        }
    }

    if (redCandle && !bDoji && confirmed)
    {
        for (int k = 1; k <= backscan; ++k)
        {
            const int idx = i - k;
            if (idx < 0) break;

            if (BrightGreenAt(idx)) break;

            const bool greenAt = sc.Close[idx] > sc.Open[idx];
            if (waveState == DOWN_TREND && greenAt) break;

            const bool redAt = sc.Close[idx] < sc.Open[idx];
            if (waveState == UP_TREND && sc.Open[i] <= sc.Close[idx] && redAt)
            {
                noOverlapRed = true;
                brightRed = true;
                waveState = DOWN_TREND;
                break;
            }
        }

        if (i > 0 && (sc.Close[i - 1] < sc.Open[i - 1]) && sc.Open[i] < sc.Close[i - 1])
        {
            waveState = DOWN_TREND;
            brightRed = true;
        }
    }

    sc.SetPersistentInt(17000 + i, waveState);
    sc.SetPersistentInt(18000 + i, brightGreen ? 1 : 0);
    sc.SetPersistentInt(19000 + i, brightRed ? 1 : 0);

    const bool buyChar  = noOverlapGreen && (waveStatePrev == DOWN_TREND);
    const bool sellChar = noOverlapRed   && (waveStatePrev == UP_TREND);

    // ============================
    // PLOTS (offset now works because we use DRAWSTYLE_POINT)
    // ============================
    BuyWeak[i] = SellWeak[i] = BuyStrong[i] = SellStrong[i] = 0.0f;

    if (buyChar)
    {
        const float y = sc.Low[i] - yOff;
        if (bBigBuy1) BuyStrong[i] = y;
        else          BuyWeak[i]   = y;
    }

    if (sellChar)
    {
        const float y = sc.High[i] + yOff;
        if (bBigSell1) SellStrong[i] = y;
        else           SellWeak[i]   = y;
    }
}