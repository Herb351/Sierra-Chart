#include "sierrachart.h"
#include <math.h>
#include <vector>

SCDLLName("TO Studies")

static int DecimalsFromTickSize(float TickSize)
{
    if (TickSize <= 0.0f)
        return 2;

    int decimals = 0;
    float t = TickSize;

    for (int i = 0; i < 8; i++)
    {
        float rounded = (float)(floor(t + 0.5f));
        if (fabs(t - rounded) < 1e-6f)
            break;

        t *= 10.0f;
        decimals++;
    }

    if (decimals < 0) decimals = 0;
    if (decimals > 8) decimals = 8;
    return decimals;
}

static int BaseInputIndexForLevel(int level /*1..10*/)
{
    // 10 inputs per line level (note: increased from 9)
    return (level - 1) * 10;
}

static void Tokenize(const SCString& s, std::vector<SCString>& out)
{
    out.clear();

    SCString token;
    const int len = s.GetLength();

    for (int i = 0; i < len; i++)
    {
        const char c = s[i];
        const bool isDelim = (c == ',' || c == ' ' || c == '\t' || c == '\r' || c == '\n');

        if (isDelim)
        {
            if (token.GetLength() > 0)
            {
                out.push_back(token);
                token = "";
            }
            continue;
        }

        char ch[2] = { c, 0 };
        token.Append(ch);
    }

    if (token.GetLength() > 0)
        out.push_back(token);
}

static bool TryParseFloat(const SCString& s, float& out)
{
    const char* p = s.GetChars();
    if (p == nullptr || *p == 0)
        return false;

    out = (float)atof(p);
    return true;
}

SCSFExport scsf_TOsStraightLines(SCStudyInterfaceRef sc)
{
    // Zones inputs
    const int IN_ZONES_TEXT = 0;
    const int IN_ZONES_ENABLE = 1;
    const int IN_ZONES_FILL_COLOR = 2;
    const int IN_ZONES_BORDER_COLOR = 3;
    const int IN_ZONES_TRANSPARENCY = 4;
    const int IN_ZONES_BORDER_WIDTH = 5;

    const int FIRST_LINE_INPUT = 20;

    if (sc.SetDefaults)
    {
        sc.GraphName = "TO's Straight Lines";
        sc.StudyDescription =
            "Zones (paste: top,<p>,bottom,<p>,...) + up to 10 labeled lines.\n"
            "Line labels are right-justified inside chart, under the line: TITLE, PRICE.";
        sc.AutoLoop = 0;
        sc.GraphRegion = 0;

        // Zones
        sc.Input[IN_ZONES_TEXT].Name = "Zones Text (paste: top,<p>,bottom,<p>, ...)";
        sc.Input[IN_ZONES_TEXT].SetString("");

        sc.Input[IN_ZONES_ENABLE].Name = "Enable Zones";
        sc.Input[IN_ZONES_ENABLE].SetYesNo(1);

        sc.Input[IN_ZONES_FILL_COLOR].Name = "Zones Fill Color";
        sc.Input[IN_ZONES_FILL_COLOR].SetColor(RGB(30, 90, 160));

        sc.Input[IN_ZONES_BORDER_COLOR].Name = "Zones Border Color";
        sc.Input[IN_ZONES_BORDER_COLOR].SetColor(RGB(30, 90, 160));

        sc.Input[IN_ZONES_TRANSPARENCY].Name = "Zones Fill Transparency (0=opaque, 100=transparent)";
        sc.Input[IN_ZONES_TRANSPARENCY].SetInt(75);

        sc.Input[IN_ZONES_BORDER_WIDTH].Name = "Zones Border Width";
        sc.Input[IN_ZONES_BORDER_WIDTH].SetInt(1);

        // 10 line levels
        for (int level = 1; level <= 10; level++)
        {
            const int base = FIRST_LINE_INPUT + BaseInputIndexForLevel(level);
            SCString Name;

            Name.Format("L%d Enable", level);
            sc.Input[base + 0].Name = Name;
            sc.Input[base + 0].SetYesNo(0);

            Name.Format("L%d Price", level);
            sc.Input[base + 1].Name = Name;
            sc.Input[base + 1].SetFloat(0.0f);

            Name.Format("L%d Title", level);
            sc.Input[base + 2].Name = Name;
            sc.Input[base + 2].SetString("");

            Name.Format("L%d Line Color", level);
            sc.Input[base + 3].Name = Name;
            sc.Input[base + 3].SetColor(RGB(255, 0, 0));

            Name.Format("L%d Text Color", level);
            sc.Input[base + 4].Name = Name;
            sc.Input[base + 4].SetColor(RGB(255, 0, 0));

            Name.Format("L%d Line Width", level);
            sc.Input[base + 5].Name = Name;
            sc.Input[base + 5].SetInt(2);

            Name.Format("L%d Font Size", level);
            sc.Input[base + 6].Name = Name;
            sc.Input[base + 6].SetInt(10);

            Name.Format("L%d Label Offset Below Line (ticks)", level);
            sc.Input[base + 7].Name = Name;
            sc.Input[base + 7].SetInt(2);

            // NEW: controls how far inside the chart the label sits
            Name.Format("L%d Label Right Offset (bars, negative=inside chart)", level);
            sc.Input[base + 8].Name = Name;
            sc.Input[base + 8].SetInt(-2);

            // Optional: extra vertical padding under the line (in ticks)
            Name.Format("L%d Extra Label Padding (ticks)", level);
            sc.Input[base + 9].Name = Name;
            sc.Input[base + 9].SetInt(0);
        }

        return;
    }

    const int lastIndex = sc.ArraySize > 0 ? (sc.ArraySize - 1) : 0;

    // -----------------
    // Zones
    // -----------------
    const bool zonesEnabled = sc.Input[IN_ZONES_ENABLE].GetYesNo() != 0;
    const SCString zonesText = sc.Input[IN_ZONES_TEXT].GetString();
    const COLORREF fillColor = sc.Input[IN_ZONES_FILL_COLOR].GetColor();
    const COLORREF borderColor = sc.Input[IN_ZONES_BORDER_COLOR].GetColor();

    int fillTransparency = sc.Input[IN_ZONES_TRANSPARENCY].GetInt();
    if (fillTransparency < 0) fillTransparency = 0;
    if (fillTransparency > 100) fillTransparency = 100;

    const int borderWidth = sc.Input[IN_ZONES_BORDER_WIDTH].GetInt();

    const int ZONE_BASE = 30000;
    const int MAX_ZONES = 120;

    if (!zonesEnabled || zonesText.GetLength() == 0)
    {
        for (int i = 1; i <= MAX_ZONES; i++)
            sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, ZONE_BASE + i);
    }
    else
    {
        std::vector<SCString> tokens;
        Tokenize(zonesText, tokens);

        int zoneCount = 0;
        float pendingTop = 0.0f;
        bool haveTop = false;

        for (size_t i = 0; i + 1 < tokens.size(); i++)
        {
            const SCString key = tokens[i];

            const bool isTop = (key.CompareNoCase("top") == 0 || key.CompareNoCase("top:") == 0);
            const bool isBottom = (key.CompareNoCase("bottom") == 0 || key.CompareNoCase("bottom:") == 0);

            if (isTop)
            {
                float v = 0.0f;
                if (TryParseFloat(tokens[i + 1], v))
                {
                    pendingTop = v;
                    haveTop = true;
                    i++;
                }
            }
            else if (isBottom && haveTop)
            {
                float bottom = 0.0f;
                if (TryParseFloat(tokens[i + 1], bottom))
                {
                    float top = pendingTop;

                    if (bottom > top)
                    {
                        float tmp = bottom;
                        bottom = top;
                        top = tmp;
                    }

                    zoneCount++;
                    if (zoneCount > MAX_ZONES)
                        break;

                    s_UseTool r;
                    r.Clear();
                    r.ChartNumber = sc.ChartNumber;
                    r.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
                    r.LineNumber = ZONE_BASE + zoneCount;
                    r.AddMethod = UTAM_ADD_OR_ADJUST;

                    r.BeginIndex = 0;
                    r.EndIndex = lastIndex;
                    r.BeginValue = top;
                    r.EndValue = bottom;

                    // Border
                    r.Color = borderColor;
                    r.LineWidth = borderWidth;

                    // Fill (common ACSIL field for rectangle highlight)
                    r.SecondaryColor = fillColor;
                    r.TransparencyLevel = fillTransparency;

                    r.DrawUnderneathMainGraph = 1;

                    sc.UseTool(r);

                    haveTop = false;
                    i++;
                }
            }
        }

        for (int i = zoneCount + 1; i <= MAX_ZONES; i++)
            sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, ZONE_BASE + i);
    }

    // -----------------
    // 10 Lines + Labels
    // -----------------
    const int LINE_BASE = 20000;
    const int TEXT_BASE = 21000;
    const int decimals = DecimalsFromTickSize(sc.TickSize);

    for (int level = 1; level <= 10; level++)
    {
        const int base = FIRST_LINE_INPUT + BaseInputIndexForLevel(level);

        const bool enabled = sc.Input[base + 0].GetYesNo() != 0;
        const float price = sc.Input[base + 1].GetFloat();
        const SCString title = sc.Input[base + 2].GetString();
        const COLORREF lineColor = sc.Input[base + 3].GetColor();
        const COLORREF textColor = sc.Input[base + 4].GetColor();
        const int lineWidth = sc.Input[base + 5].GetInt();
        const int fontSize = sc.Input[base + 6].GetInt();
        const int offsetTicks = sc.Input[base + 7].GetInt();
        const int rightOffsetBars = sc.Input[base + 8].GetInt();
        const int extraPadTicks = sc.Input[base + 9].GetInt();

        const int lineNumber = LINE_BASE + level;
        const int textNumber = TEXT_BASE + level;

        if (!enabled || price == 0.0f)
        {
            sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, lineNumber);
            sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, textNumber);
            continue;
        }

        // Horizontal line: no built-in price label
        {
            s_UseTool line;
            line.Clear();
            line.ChartNumber = sc.ChartNumber;
            line.DrawingType = DRAWING_HORIZONTALLINE;
            line.LineNumber = lineNumber;
            line.AddMethod = UTAM_ADD_OR_ADJUST;

            line.BeginValue = price;
            line.Color = lineColor;
            line.LineWidth = lineWidth;

            line.ShowPrice = 0;   // keep it off
            line.ShowLabels = 0;

            sc.UseTool(line);
        }

        // Label under line: "Title, Price" inside chart near right edge
        {
            SCString priceStr;
            priceStr.Format("%.*f", decimals, price);

            SCString label;
            if (title.GetLength() > 0)
                label.Format("%s, %s", title.GetChars(), priceStr.GetChars());
            else
                label = priceStr;

            float y = price;
            if (sc.TickSize > 0.0f)
                y = price - ((offsetTicks + extraPadTicks) * sc.TickSize);

            int anchorIndex = lastIndex + rightOffsetBars;
            if (anchorIndex < 0) anchorIndex = 0;
            if (anchorIndex > lastIndex) anchorIndex = lastIndex;

            s_UseTool text;
            text.Clear();
            text.ChartNumber = sc.ChartNumber;
            text.DrawingType = DRAWING_TEXT;
            text.LineNumber = textNumber;
            text.AddMethod = UTAM_ADD_OR_ADJUST;

            text.BeginIndex = anchorIndex;
            text.BeginValue = y;

            text.Text = label;
            text.TextColor = textColor;
            text.FontSize = fontSize;

            // Right-justified text
            text.TextAlignment = DT_RIGHT;

            sc.UseTool(text);
        }
    }
}