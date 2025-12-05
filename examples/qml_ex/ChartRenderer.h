#pragma once

#include "ChartDataManager.h"
#include <imgui.h>
#include <implot.h>

class ChartRenderer
{
public:
    static void render(ChartDataManager* manager);

private:
    static void renderLine(const ChartSeries& series);
    static void renderBars(const ChartSeries& series);
    static void renderScatter(const ChartSeries& series);
    static void renderShaded(const ChartSeries& series);
    static void renderCandles(const ChartSeries& series);

    static ImVec4 qColorToImVec4(const QColor& color);
};
