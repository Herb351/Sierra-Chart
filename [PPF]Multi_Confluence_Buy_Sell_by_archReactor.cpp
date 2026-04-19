#include "sierrachart.h"

SCDLLName("[PPF]Multi_Confulence Buy_Sell by archReactor")

enum SourceChoice
{
    SRC_CLOSE = 0,
    SRC_OPEN = 1,
    SRC_HIGH = 2,
    SRC_LOW = 3,
    SRC_HL2 = 4,
    SRC_HLC3 = 5,
    SRC_OHLC4 = 6
};

enum FireMode
{
    FIRE_ON_BAR_CLOSE = 0,
    FIRE_INTRABAR = 1
};

static float GetSourceAt(SCStudyInterfaceRef sc, int Index, int Choice)
{
    switch (Choice)
    {
        default:
        case SRC_CLOSE:  return sc.Close[Index];
        case SRC_OPEN:   return sc.Open[Index];
        case SRC_HIGH:   return sc.High[Index];
        case SRC_LOW:    return sc.Low[Index];
        case SRC_HL2:    return (sc.High[Index] + sc.Low[Index]) * 0.5f;
        case SRC_HLC3:   return (sc.High[Index] + sc.Low[Index] + sc.Close[Index]) / 3.0f;
        case SRC_OHLC4:  return (sc.Open[Index] + sc.High[Index] + sc.Low[Index] + sc.Close[Index]) * 0.25f;
    }
}

static inline float Log10Safe(float x)
{
    if (x <= 0.0f) x = 1e-10f;
    return log10f(x);
}

static float ZScoreAt(int Index, const SCFloatArrayRef& Series, const SCFloatArrayRef& Mean, const SCFloatArrayRef& Stdev)
{
    float sd = Stdev[Index];
    if (sd == 0.0f) sd = 1.0f;
    return (Series[Index] - Mean[Index]) / sd;
}

static float SlopeLR(const SCFloatArrayRef& Series, int Index, int Len)
{
    if (Len <= 1 || Index - (Len - 1) < 0)
        return 0.0f;

    double sumX = (double)Len * (Len - 1) / 2.0;
    double sumX2 = (double)Len * (Len - 1) * (2.0 * Len - 1.0) / 6.0;

    double sumY = 0.0;
    double sumXY = 0.0;

    for (int i = 0; i < Len; ++i)
    {
        int bar = Index - (Len - 1) + i;
        double y = Series[bar];
        sumY += y;
        sumXY += (double)i * y;
    }

    double denom = (double)Len * sumX2 - sumX * sumX;
    if (denom == 0.0)
        return 0.0f;

    double num = (double)Len * sumXY - sumX * sumY;
    return (float)(num / denom);
}

SCSFExport scsf_MultiIndicatorConfluenceMLSignal(SCStudyInterfaceRef sc)
{
    // ---- Inputs ----
    SCInputRef In_SourceChoice = sc.Input[0];
    SCInputRef In_FireMode = sc.Input[1];

    SCInputRef In_Per = sc.Input[2];
    SCInputRef In_Mult = sc.Input[3];

    SCInputRef In_EnableRSI = sc.Input[4];
    SCInputRef In_EnableMACD = sc.Input[5];
    SCInputRef In_EnableVolume = sc.Input[6];
    SCInputRef In_EnableATRFilter = sc.Input[7];
    SCInputRef In_EnableChopFilter = sc.Input[8];

    SCInputRef In_RSI_Period = sc.Input[9];
    SCInputRef In_RSI_OB = sc.Input[10];
    SCInputRef In_RSI_OS = sc.Input[11];

    SCInputRef In_MACD_Fast = sc.Input[12];
    SCInputRef In_MACD_Slow = sc.Input[13];
    SCInputRef In_MACD_SignalLen = sc.Input[14];
    SCInputRef In_MACD_UseSMAOsc = sc.Input[15]; // 0=EMA,1=SMA
    SCInputRef In_MACD_UseSMASig = sc.Input[16]; // 0=EMA,1=SMA

    SCInputRef In_VolMA_Len = sc.Input[17];

    SCInputRef In_ATR_Period = sc.Input[18];
    SCInputRef In_ATR_FilterMA = sc.Input[19];
    SCInputRef In_ATR_ThreshMult = sc.Input[20];
    SCInputRef In_VolumeMult = sc.Input[21];

    SCInputRef In_Chop_Period = sc.Input[22];
    SCInputRef In_Chop_Threshold = sc.Input[23];

    SCInputRef In_ML_Model = sc.Input[24]; // 0=None, 1=Lorentzian, 2=RNN
    SCInputRef In_LorentzLookback = sc.Input[25];
    SCInputRef In_LorentzPredLen = sc.Input[26];
    SCInputRef In_LorentzProfitThresh = sc.Input[27]; // percent
    SCInputRef In_LorentzNormPeriod = sc.Input[28];
    SCInputRef In_RNN_SeqLen = sc.Input[29];
    SCInputRef In_RNN_MemoryEMA = sc.Input[30];

    // ---- Subgraphs ----
    SCSubgraphRef SG_Buy = sc.Subgraph[0];
    SCSubgraphRef SG_Sell = sc.Subgraph[1];

    SCSubgraphRef SG_Filt = sc.Subgraph[2];
    SCSubgraphRef SG_SmRng = sc.Subgraph[3];
    SCSubgraphRef SG_UpCount = sc.Subgraph[4];
    SCSubgraphRef SG_DownCount = sc.Subgraph[5];
    SCSubgraphRef SG_CondIni = sc.Subgraph[6];

    SCSubgraphRef SG_RSI = sc.Subgraph[7];

    SCSubgraphRef SG_FastMA = sc.Subgraph[8];
    SCSubgraphRef SG_SlowMA = sc.Subgraph[9];
    SCSubgraphRef SG_MACD = sc.Subgraph[10];
    SCSubgraphRef SG_MACDSignal = sc.Subgraph[11];
    SCSubgraphRef SG_Hist = sc.Subgraph[12];

    SCSubgraphRef SG_ATR = sc.Subgraph[13];
    SCSubgraphRef SG_ATRMA = sc.Subgraph[14];

    SCSubgraphRef SG_VolMA = sc.Subgraph[15];
    SCSubgraphRef SG_CI = sc.Subgraph[16];

    SCSubgraphRef SG_RSIMean = sc.Subgraph[17];
    SCSubgraphRef SG_RSIStd = sc.Subgraph[18];
    SCSubgraphRef SG_MACDMean = sc.Subgraph[19];
    SCSubgraphRef SG_MACDStd = sc.Subgraph[20];

    SCSubgraphRef SG_RSISeqSlope = sc.Subgraph[21];
    SCSubgraphRef SG_HistSeqSlope = sc.Subgraph[22];
    SCSubgraphRef SG_RSIMemory = sc.Subgraph[23];
    SCSubgraphRef SG_HistMemory = sc.Subgraph[24];

    // Temps
    SCSubgraphRef SG_AbsDiff = sc.Subgraph[25];
    SCSubgraphRef SG_AvRng = sc.Subgraph[26];
    SCSubgraphRef SG_SmRngUnmult = sc.Subgraph[27];

    SCSubgraphRef SG_ATR1 = sc.Subgraph[28];
    SCSubgraphRef SG_ATR1Sum = sc.Subgraph[29];
    SCSubgraphRef SG_AbsSrcDiff = sc.Subgraph[30];
    SCSubgraphRef SG_AbsSrcDiffSum = sc.Subgraph[31];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Multi-Indicator Confluence ML Signal (Converted)";
        sc.AutoLoop = 1;
        sc.GraphRegion = 0;

        // Needed for Intrabar mode and to print immediately when a bar closes.
        sc.UpdateAlways = 1;

        In_SourceChoice.Name = "Source";
        In_SourceChoice.SetCustomInputStrings("Close;Open;High;Low;HL2;HLC3;OHLC4");
        In_SourceChoice.SetCustomInputIndex(0);

        In_FireMode.Name = "Fire Signals";
        In_FireMode.SetCustomInputStrings("On Bar Close;Intrabar");
        In_FireMode.SetCustomInputIndex(FIRE_ON_BAR_CLOSE);

        In_Per.Name = "Sampling Period";
        In_Per.SetInt(5);
        In_Per.SetIntLimits(1, 1000);

        In_Mult.Name = "Range Multiplier";
        In_Mult.SetFloat(1.2f);
        In_Mult.SetFloatLimits(0.1f, 100.0f);

        In_EnableRSI.Name = "Enable RSI";
        In_EnableRSI.SetYesNo(0);

        In_EnableMACD.Name = "Enable MACD";
        In_EnableMACD.SetYesNo(1);

        In_EnableVolume.Name = "Enable Volume";
        In_EnableVolume.SetYesNo(0);

        In_EnableATRFilter.Name = "Enable Volatility (ATR) Filter";
        In_EnableATRFilter.SetYesNo(1);

        In_EnableChopFilter.Name = "Enable Choppiness Filter";
        In_EnableChopFilter.SetYesNo(0);

        In_RSI_Period.Name = "RSI Period";
        In_RSI_Period.SetInt(14);

        In_RSI_OB.Name = "RSI OB threshold";
        In_RSI_OB.SetInt(55);

        In_RSI_OS.Name = "RSI OS threshold";
        In_RSI_OS.SetInt(45);

        In_MACD_Fast.Name = "MACD Fast Length";
        In_MACD_Fast.SetInt(6);

        In_MACD_Slow.Name = "MACD Slow Length";
        In_MACD_Slow.SetInt(13);

        In_MACD_SignalLen.Name = "MACD Signal Length";
        In_MACD_SignalLen.SetInt(5);

        In_MACD_UseSMAOsc.Name = "MACD Oscillator MA Type (0=EMA,1=SMA)";
        In_MACD_UseSMAOsc.SetInt(0);

        In_MACD_UseSMASig.Name = "MACD Signal MA Type (0=EMA,1=SMA)";
        In_MACD_UseSMASig.SetInt(0);

        In_VolMA_Len.Name = "Volume MA Length";
        In_VolMA_Len.SetInt(10);

        In_ATR_Period.Name = "ATR Period";
        In_ATR_Period.SetInt(14);

        In_ATR_FilterMA.Name = "ATR Filter MA Period";
        In_ATR_FilterMA.SetInt(21);

        In_ATR_ThreshMult.Name = "ATR Threshold Multiplier";
        In_ATR_ThreshMult.SetFloat(1.0f);

        In_VolumeMult.Name = "Volume MA Multiplier";
        In_VolumeMult.SetFloat(5.0f);

        In_Chop_Period.Name = "Choppiness Period";
        In_Chop_Period.SetInt(21);

        In_Chop_Threshold.Name = "Choppiness Threshold";
        In_Chop_Threshold.SetFloat(40.0f);

        In_ML_Model.Name = "ML Model (0=None, 1=Lorentzian, 2=RNN)";
        In_ML_Model.SetInt(2);

        In_LorentzLookback.Name = "Lorentzian Lookback";
        In_LorentzLookback.SetInt(300);

        In_LorentzPredLen.Name = "Lorentzian Prediction Length";
        In_LorentzPredLen.SetInt(5);

        In_LorentzProfitThresh.Name = "Lorentzian Profit Threshold (%)";
        In_LorentzProfitThresh.SetFloat(0.01f);

        In_LorentzNormPeriod.Name = "Lorentzian Normalization Period";
        In_LorentzNormPeriod.SetInt(50);

        In_RNN_SeqLen.Name = "RNN Sequence Length";
        In_RNN_SeqLen.SetInt(4);

        In_RNN_MemoryEMA.Name = "RNN Memory Period (EMA)";
        In_RNN_MemoryEMA.SetInt(8);

        SG_Buy.Name = "BUY";
        SG_Buy.DrawStyle = DRAWSTYLE_ARROWUP;
        SG_Buy.PrimaryColor = RGB(0, 255, 0);
        SG_Buy.LineWidth = 2;
        SG_Buy.DrawZeros = false;

        SG_Sell.Name = "SELL";
        SG_Sell.DrawStyle = DRAWSTYLE_ARROWDOWN;
        SG_Sell.PrimaryColor = RGB(255, 128, 0);
        SG_Sell.LineWidth = 2;
        SG_Sell.DrawZeros = false;

        // Hide internal subgraphs
        for (int i = 2; i <= 31; ++i)
            sc.Subgraph[i].DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int Index = sc.Index;

    // --- Determine whether signals are allowed to fire on this bar ---
    const int fireMode = In_FireMode.GetIndex();

    bool BarClosed = (Index < sc.ArraySize - 1); // universal fallback

    // Use bar-closed status if available in your build (will compile if symbols exist)
    // If your Sierra version does not define these, comment this block out.
    {
        int bhcs = sc.GetBarHasClosedStatus(Index);
        if (bhcs == BHCS_BAR_HAS_CLOSED)
            BarClosed = true;
        else if (bhcs == BHCS_BAR_HAS_NOT_CLOSED)
            BarClosed = false;
    }

    const bool AllowFireThisBar = (fireMode == FIRE_INTRABAR) ? true : BarClosed;

    if (!AllowFireThisBar)
    {
        SG_Buy[Index] = 0.0f;
        SG_Sell[Index] = 0.0f;
    }

    // ---- Selectable Source for Range Filter ----
    const int srcChoice = In_SourceChoice.GetIndex();
    const float src = GetSourceAt(sc, Index, srcChoice);
    const float srcPrev = (Index > 0) ? GetSourceAt(sc, Index - 1, srcChoice) : src;

    // ---- Smooth Range (Pine smoothrng) ----
    SG_AbsDiff[Index] = (Index > 0) ? fabsf(src - srcPrev) : 0.0f;

    const int per = In_Per.GetInt();
    const float mult = In_Mult.GetFloat();
    const int wper = per * 2 - 1;

    sc.ExponentialMovAvg(SG_AbsDiff, SG_AvRng, per);
    sc.ExponentialMovAvg(SG_AvRng, SG_SmRngUnmult, wper);
    SG_SmRng[Index] = SG_SmRngUnmult[Index] * mult;

    // ---- Range Filter ----
    if (Index == 0)
    {
        SG_Filt[Index] = src;
        SG_UpCount[Index] = 0.0f;
        SG_DownCount[Index] = 0.0f;
        SG_CondIni[Index] = 0.0f;
    }
    else
    {
        float prevFilt = SG_Filt[Index - 1];
        float r = SG_SmRng[Index];

        float filt;
        if (src > prevFilt)
        {
            float candidate = src - r;
            filt = (candidate < prevFilt) ? prevFilt : candidate;
        }
        else
        {
            float candidate = src + r;
            filt = (candidate > prevFilt) ? prevFilt : candidate;
        }
        SG_Filt[Index] = filt;

        float up = SG_UpCount[Index - 1];
        float dn = SG_DownCount[Index - 1];

        if (SG_Filt[Index] > SG_Filt[Index - 1]) up = up + 1.0f;
        else if (SG_Filt[Index] < SG_Filt[Index - 1]) up = 0.0f;

        if (SG_Filt[Index] < SG_Filt[Index - 1]) dn = dn + 1.0f;
        else if (SG_Filt[Index] > SG_Filt[Index - 1]) dn = 0.0f;

        SG_UpCount[Index] = up;
        SG_DownCount[Index] = dn;
    }

    bool longCond = false, shortCond = false;
    if (Index > 0)
    {
        float up = SG_UpCount[Index];
        float dn = SG_DownCount[Index];

        longCond =
            (src > SG_Filt[Index] && src > srcPrev && up > 0.0f) ||
            (src > SG_Filt[Index] && src < srcPrev && up > 0.0f);

        shortCond =
            (src < SG_Filt[Index] && src < srcPrev && dn > 0.0f) ||
            (src < SG_Filt[Index] && src > srcPrev && dn > 0.0f);
    }

    int prevCondIni = (Index > 0) ? (int)SG_CondIni[Index - 1] : 0;
    int condIni = prevCondIni;
    if (longCond) condIni = 1;
    else if (shortCond) condIni = -1;
    SG_CondIni[Index] = (float)condIni;

    bool longCondition = longCond && (prevCondIni == -1);
    bool shortCondition = shortCond && (prevCondIni == 1);

    // ---- RSI (Wilder's, matches TradingView style) ----
    sc.RSI(sc.Close, SG_RSI, MOVAVGTYPE_WILDERS, In_RSI_Period.GetInt());
    bool rsiSignal = (SG_RSI[Index] > (float)In_RSI_OB.GetInt()) || (SG_RSI[Index] < (float)In_RSI_OS.GetInt());

    // ---- MACD ----
    const int macdFast = In_MACD_Fast.GetInt();
    const int macdSlow = In_MACD_Slow.GetInt();
    const int macdSigLen = In_MACD_SignalLen.GetInt();
    const bool oscSMA = (In_MACD_UseSMAOsc.GetInt() == 1);
    const bool sigSMA = (In_MACD_UseSMASig.GetInt() == 1);

    if (oscSMA)
    {
        sc.SimpleMovAvg(sc.Close, SG_FastMA, macdFast);
        sc.SimpleMovAvg(sc.Close, SG_SlowMA, macdSlow);
    }
    else
    {
        sc.ExponentialMovAvg(sc.Close, SG_FastMA, macdFast);
        sc.ExponentialMovAvg(sc.Close, SG_SlowMA, macdSlow);
    }

    SG_MACD[Index] = SG_FastMA[Index] - SG_SlowMA[Index];

    if (sigSMA)
        sc.SimpleMovAvg(SG_MACD, SG_MACDSignal, macdSigLen);
    else
        sc.ExponentialMovAvg(SG_MACD, SG_MACDSignal, macdSigLen);

    SG_Hist[Index] = SG_MACD[Index] - SG_MACDSignal[Index];

    bool macdBuyCond = (SG_MACD[Index] > SG_MACDSignal[Index]) && (SG_Hist[Index] > 0.0f);
    bool macdSellCond = (SG_MACD[Index] < SG_MACDSignal[Index]) && (SG_Hist[Index] < 0.0f);

    // ---- Volume ----
    sc.SimpleMovAvg(sc.Volume, SG_VolMA, In_VolMA_Len.GetInt());
    bool volumeAbove = sc.Volume[Index] > SG_VolMA[Index] * In_VolumeMult.GetFloat();

    // ---- ATR ----
    sc.ATR(sc.BaseDataIn, SG_ATR, In_ATR_Period.GetInt(), MOVAVGTYPE_WILDERS);
    sc.SimpleMovAvg(SG_ATR, SG_ATRMA, In_ATR_FilterMA.GetInt());
    bool atrCondition = !In_EnableATRFilter.GetYesNo() ? true : (SG_ATR[Index] > SG_ATRMA[Index] * In_ATR_ThreshMult.GetFloat());

    // ---- Choppiness Index (mirror Pine using HLC3) ----
    sc.ATR(sc.BaseDataIn, SG_ATR1, 1, MOVAVGTYPE_WILDERS);
    sc.Summation(SG_ATR1, SG_ATR1Sum, In_Chop_Period.GetInt());

    float srcCI = GetSourceAt(sc, Index, SRC_HLC3);
    float srcCIPrev = (Index > 0) ? GetSourceAt(sc, Index - 1, SRC_HLC3) : srcCI;
    SG_AbsSrcDiff[Index] = fabsf(srcCI - srcCIPrev);
    sc.Summation(SG_AbsSrcDiff, SG_AbsSrcDiffSum, In_Chop_Period.GetInt());

    float atr_sum = SG_ATR1Sum[Index];
    float tr_sum = SG_AbsSrcDiffSum[Index];
    float ci = 0.0f;
    int ciLen = In_Chop_Period.GetInt();
    if (Index >= ciLen && tr_sum > 0.0f)
        ci = 100.0f * (Log10Safe(atr_sum / tr_sum) / Log10Safe((float)ciLen));
    SG_CI[Index] = ci;

    bool choppinessCondition = !In_EnableChopFilter.GetYesNo() ? true : (SG_CI[Index] < In_Chop_Threshold.GetFloat());

    // ---- Common / buy / sell ----
    bool commonSignal =
        (!In_EnableRSI.GetYesNo() ? true : rsiSignal) &&
        (!In_EnableVolume.GetYesNo() ? true : volumeAbove) &&
        atrCondition &&
        choppinessCondition;

    bool buySignal = longCondition && commonSignal && (!In_EnableMACD.GetYesNo() ? true : macdBuyCond);
    bool sellSignal = shortCondition && commonSignal && (!In_EnableMACD.GetYesNo() ? true : macdSellCond);

    // ---- ML Filter ----
    int mlModel = In_ML_Model.GetInt();
    int mlSignal = 0;

    int normP = In_LorentzNormPeriod.GetInt();
    sc.SimpleMovAvg(SG_RSI, SG_RSIMean, normP);
    sc.StdDeviation(SG_RSI, SG_RSIStd, normP);
    sc.SimpleMovAvg(SG_MACD, SG_MACDMean, normP);
    sc.StdDeviation(SG_MACD, SG_MACDStd, normP);

    if (mlModel == 1)
    {
        int lookback = In_LorentzLookback.GetInt();
        int predLen = In_LorentzPredLen.GetInt();
        float profitThresh = In_LorentzProfitThresh.GetFloat();

        if (Index > lookback + normP + 20)
        {
            float rsi_z_now = ZScoreAt(Index, SG_RSI, SG_RSIMean, SG_RSIStd);
            float macd_z_now = ZScoreAt(Index, SG_MACD, SG_MACDMean, SG_MACDStd);

            float minDist = 1e30f;
            int bestOffset = -1;

            for (int i = 1; i <= lookback; ++i)
            {
                int pastIndex = Index - i;
                if (pastIndex < 0) break;

                float rsi_z_p = ZScoreAt(pastIndex, SG_RSI, SG_RSIMean, SG_RSIStd);
                float macd_z_p = ZScoreAt(pastIndex, SG_MACD, SG_MACDMean, SG_MACDStd);

                float dist = logf(1.0f + fabsf(rsi_z_now - rsi_z_p))
                           + logf(1.0f + fabsf(macd_z_now - macd_z_p));

                if (dist < minDist)
                {
                    minDist = dist;
                    bestOffset = i;
                }
            }

            if (bestOffset > predLen)
            {
                float price_at_nn = sc.Close[Index - bestOffset];
                float price_after = sc.Close[Index - bestOffset + predLen];
                if (price_at_nn != 0.0f)
                {
                    float price_chg_pct = (price_after - price_at_nn) / price_at_nn * 100.0f;
                    if (price_chg_pct > profitThresh) mlSignal = 1;
                    else if (price_chg_pct < -profitThresh) mlSignal = -1;
                }
            }
        }
    }
    else if (mlModel == 2)
    {
        int seqLen = In_RNN_SeqLen.GetInt();
        int memEma = In_RNN_MemoryEMA.GetInt();

        SG_RSISeqSlope[Index] = SlopeLR(SG_RSI, Index, seqLen);
        SG_HistSeqSlope[Index] = SlopeLR(SG_Hist, Index, seqLen);

        sc.ExponentialMovAvg(SG_RSISeqSlope, SG_RSIMemory, memEma);
        sc.ExponentialMovAvg(SG_HistSeqSlope, SG_HistMemory, memEma);

        bool rnnBuy = (SG_RSIMemory[Index] > 0.0f) && (SG_HistMemory[Index] > 0.0f);
        bool rnnSell = (SG_RSIMemory[Index] < 0.0f) && (SG_HistMemory[Index] < 0.0f);

        mlSignal = rnnBuy ? 1 : (rnnSell ? -1 : 0);
    }

    bool enhancedBuy = buySignal && (mlModel == 0 || mlSignal == 1);
    bool enhancedSell = sellSignal && (mlModel == 0 || mlSignal == -1);

    // ---- Plot/fire ----
    if (AllowFireThisBar)
    {
        SG_Buy[Index] = enhancedBuy ? (sc.Low[Index] - sc.TickSize * 2.0f) : 0.0f;
        SG_Sell[Index] = enhancedSell ? (sc.High[Index] + sc.TickSize * 2.0f) : 0.0f;
    }
}