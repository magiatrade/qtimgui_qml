#include "ChartRenderer.h"

ImVec4 ChartRenderer::qColorToImVec4(const QColor& color)
{
    return ImVec4(
        color.redF(),
        color.greenF(),
        color.blueF(),
        color.alphaF()
    );
}

void ChartRenderer::render(ChartDataManager* manager)
{
    // Create a DockSpace over the entire viewport
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (!manager) {
        // Show placeholder when no manager
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Chart");
        ImGui::Text("No chart data available");
        ImGui::End();
        return;
    }

    const auto& allSeries = manager->getAllSeries();

    // Main chart window
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);

    ImGui::Begin("Interactive Chart", nullptr,
                 ImGuiWindowFlags_NoCollapse);

    // Chart info
    ImGui::Text("Active Series: %d", static_cast<int>(allSeries.size()));
    ImGui::SameLine();
    ImGui::TextDisabled("(Drag indicators from the toolbar to add more)");

    ImGui::Separator();

    if (allSeries.empty()) {
        // Show empty state
        ImVec2 availSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Drop indicators here to create charts");
        ImGui::SetCursorPos(ImVec2(
            (availSize.x - textSize.x) * 0.5f + ImGui::GetCursorPosX(),
            (availSize.y - textSize.y) * 0.5f + ImGui::GetCursorPosY()
        ));
        ImGui::TextDisabled("Drop indicators here to create charts");
    } else {
        // Render the plot
        if (ImPlot::BeginPlot("##Chart", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("X", "Y");
            ImPlot::SetupAxesLimits(0, 100, -5, 5, ImGuiCond_FirstUseEver);

            for (const auto& series : allSeries) {
                if (!series.visible) continue;

                ImPlot::PushStyleColor(ImPlotCol_Line, qColorToImVec4(series.color));
                ImPlot::PushStyleColor(ImPlotCol_Fill, qColorToImVec4(series.color));
                ImPlot::PushStyleColor(ImPlotCol_MarkerFill, qColorToImVec4(series.color));
                ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, qColorToImVec4(series.color));

                switch (series.type) {
                    case IndicatorType::Line:
                        renderLine(series);
                        break;
                    case IndicatorType::Bars:
                        renderBars(series);
                        break;
                    case IndicatorType::Scatter:
                        renderScatter(series);
                        break;
                    case IndicatorType::Shaded:
                        renderShaded(series);
                        break;
                    case IndicatorType::Candles:
                        renderCandles(series);
                        break;
                }

                ImPlot::PopStyleColor(4);
            }

            ImPlot::EndPlot();
        }
    }

    ImGui::End();

    // Legend window
    if (!allSeries.empty()) {
        ImGui::SetNextWindowPos(ImVec2(840, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);

        ImGui::Begin("Legend", nullptr, ImGuiWindowFlags_NoCollapse);

        for (size_t i = 0; i < allSeries.size(); ++i) {
            const auto& series = allSeries[i];
            ImVec4 color = qColorToImVec4(series.color);

            if (!series.visible) {
                color.w = 0.3f;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::BulletText("%s", series.name.toStdString().c_str());
            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Type: %s\nPoints: %d",
                    series.type == IndicatorType::Candles ? "Candles" :
                    series.type == IndicatorType::Line ? "Line" :
                    series.type == IndicatorType::Bars ? "Bars" :
                    series.type == IndicatorType::Scatter ? "Scatter" : "Shaded",
                    static_cast<int>(series.xData.size()));
            }
        }

        ImGui::End();
    }
}

void ChartRenderer::renderLine(const ChartSeries& series)
{
    if (series.xData.empty()) return;
    ImPlot::PlotLine(series.name.toStdString().c_str(),
                     series.xData.data(),
                     series.yData.data(),
                     static_cast<int>(series.xData.size()));
}

void ChartRenderer::renderBars(const ChartSeries& series)
{
    if (series.xData.empty()) return;
    ImPlot::PlotBars(series.name.toStdString().c_str(),
                     series.xData.data(),
                     series.yData.data(),
                     static_cast<int>(series.xData.size()),
                     0.5);
}

void ChartRenderer::renderScatter(const ChartSeries& series)
{
    if (series.xData.empty()) return;
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4);
    ImPlot::PlotScatter(series.name.toStdString().c_str(),
                        series.xData.data(),
                        series.yData.data(),
                        static_cast<int>(series.xData.size()));
}

void ChartRenderer::renderShaded(const ChartSeries& series)
{
    if (series.xData.empty()) return;

    // Create a zero line for shading
    std::vector<float> zeros(series.yData.size(), 0.0f);

    ImPlot::PlotShaded(series.name.toStdString().c_str(),
                       series.xData.data(),
                       series.yData.data(),
                       zeros.data(),
                       static_cast<int>(series.xData.size()));

    // Also draw the line on top
    ImPlot::PlotLine(series.name.toStdString().c_str(),
                     series.xData.data(),
                     series.yData.data(),
                     static_cast<int>(series.xData.size()));
}

void ChartRenderer::renderCandles(const ChartSeries& series)
{
    if (series.xData.empty() || series.open.empty()) return;

    // ImPlot doesn't have built-in candlestick, so we draw manually
    float width = 0.4f;

    for (size_t i = 0; i < series.xData.size(); ++i) {
        float x = series.xData[i];
        float o = series.open[i];
        float h = series.high[i];
        float l = series.low[i];
        float c = series.close[i];

        ImU32 color = c >= o ?
            IM_COL32(0, 200, 0, 255) :   // Green for bullish
            IM_COL32(200, 0, 0, 255);    // Red for bearish

        // Draw the wick (high-low line)
        float xs[2] = {x, x};
        float ys[2] = {l, h};
        ImPlot::PushStyleColor(ImPlotCol_Line, color);
        ImPlot::PlotLine("##wick", xs, ys, 2);
        ImPlot::PopStyleColor();

        // Draw the body as a filled rectangle
        float bodyTop = std::max(o, c);
        float bodyBottom = std::min(o, c);

        float rectXs[5] = {x - width/2, x + width/2, x + width/2, x - width/2, x - width/2};
        float rectYs[5] = {bodyBottom, bodyBottom, bodyTop, bodyTop, bodyBottom};

        ImPlot::PushStyleColor(ImPlotCol_Fill, color);
        ImPlot::PushStyleColor(ImPlotCol_Line, color);
        ImPlot::PlotShaded("##body", rectXs, rectYs, 5);
        ImPlot::PopStyleColor(2);
    }
}
