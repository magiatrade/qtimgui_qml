#include "ChartDataManager.h"
#include <cmath>
#include <QDateTime>

ChartDataManager::ChartDataManager(QObject *parent)
    : QObject(parent)
    , m_rng(static_cast<unsigned>(QDateTime::currentMSecsSinceEpoch()))
{
}

void ChartDataManager::addIndicator(const QString &typeName, float dropX, float dropY)
{
    Q_UNUSED(dropX)
    Q_UNUSED(dropY)

    ChartSeries series;
    series.type = typeFromString(typeName);
    series.color = getNextColor();
    series.name = QString("%1 #%2").arg(typeName).arg(m_nextId++);

    if (series.type == IndicatorType::Candles) {
        generateCandleData(series);
    } else {
        generateSampleData(series);
    }

    m_series.push_back(series);
    emit seriesChanged();
    emit indicatorAdded(series.name, typeName);
}

void ChartDataManager::removeIndicator(int index)
{
    if (index >= 0 && index < static_cast<int>(m_series.size())) {
        m_series.erase(m_series.begin() + index);
        emit seriesChanged();
        emit indicatorRemoved(index);
    }
}

void ChartDataManager::clearAll()
{
    m_series.clear();
    emit seriesChanged();
}

void ChartDataManager::toggleVisibility(int index)
{
    if (index >= 0 && index < static_cast<int>(m_series.size())) {
        m_series[index].visible = !m_series[index].visible;
        emit seriesChanged();
    }
}

QVariantList ChartDataManager::seriesInfo() const
{
    QVariantList list;
    for (size_t i = 0; i < m_series.size(); ++i) {
        QVariantMap info;
        info["index"] = static_cast<int>(i);
        info["name"] = m_series[i].name;
        info["type"] = typeToString(m_series[i].type);
        info["color"] = m_series[i].color;
        info["visible"] = m_series[i].visible;
        list.append(info);
    }
    return list;
}

void ChartDataManager::generateSampleData(ChartSeries& series, int pointCount)
{
    series.xData.clear();
    series.yData.clear();
    series.xData.reserve(pointCount);
    series.yData.reserve(pointCount);

    std::uniform_real_distribution<float> noiseDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> ampDist(0.5f, 2.0f);
    std::uniform_real_distribution<float> freqDist(0.02f, 0.1f);
    std::uniform_real_distribution<float> offsetDist(-2.0f, 2.0f);

    float amplitude = ampDist(m_rng);
    float frequency = freqDist(m_rng);
    float offset = offsetDist(m_rng);
    float phase = noiseDist(m_rng) * 6.28f;

    for (int i = 0; i < pointCount; ++i) {
        float x = static_cast<float>(i);
        float noise = noiseDist(m_rng);
        float y = amplitude * std::sin(frequency * x + phase) + offset + noise * 0.3f;

        series.xData.push_back(x);
        series.yData.push_back(y);
    }
}

void ChartDataManager::generateCandleData(ChartSeries& series, int pointCount)
{
    series.xData.clear();
    series.open.clear();
    series.high.clear();
    series.low.clear();
    series.close.clear();

    series.xData.reserve(pointCount);
    series.open.reserve(pointCount);
    series.high.reserve(pointCount);
    series.low.reserve(pointCount);
    series.close.reserve(pointCount);

    std::uniform_real_distribution<float> changeDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> wickDist(0.1f, 0.5f);

    float price = 100.0f;

    for (int i = 0; i < pointCount; ++i) {
        float x = static_cast<float>(i);
        float open = price;
        float change = changeDist(m_rng);
        float close = open + change;
        float upperWick = wickDist(m_rng);
        float lowerWick = wickDist(m_rng);
        float high = std::max(open, close) + upperWick;
        float low = std::min(open, close) - lowerWick;

        series.xData.push_back(x);
        series.open.push_back(open);
        series.high.push_back(high);
        series.low.push_back(low);
        series.close.push_back(close);

        price = close + changeDist(m_rng) * 0.5f;
    }
}

QColor ChartDataManager::getNextColor()
{
    static const QList<QColor> colors = {
        QColor(0x1f, 0x77, 0xb4),  // Blue
        QColor(0xff, 0x7f, 0x0e),  // Orange
        QColor(0x2c, 0xa0, 0x2c),  // Green
        QColor(0xd6, 0x27, 0x28),  // Red
        QColor(0x94, 0x67, 0xbd),  // Purple
        QColor(0x8c, 0x56, 0x4b),  // Brown
        QColor(0xe3, 0x77, 0xc2),  // Pink
        QColor(0x7f, 0x7f, 0x7f),  // Gray
        QColor(0xbc, 0xbd, 0x22),  // Olive
        QColor(0x17, 0xbe, 0xcf),  // Cyan
    };

    return colors[m_series.size() % colors.size()];
}

IndicatorType ChartDataManager::typeFromString(const QString& typeName)
{
    if (typeName == "Line") return IndicatorType::Line;
    if (typeName == "Bars") return IndicatorType::Bars;
    if (typeName == "Scatter") return IndicatorType::Scatter;
    if (typeName == "Shaded") return IndicatorType::Shaded;
    if (typeName == "Candles") return IndicatorType::Candles;
    return IndicatorType::Line;
}

QString ChartDataManager::typeToString(IndicatorType type)
{
    switch (type) {
        case IndicatorType::Line: return "Line";
        case IndicatorType::Bars: return "Bars";
        case IndicatorType::Scatter: return "Scatter";
        case IndicatorType::Shaded: return "Shaded";
        case IndicatorType::Candles: return "Candles";
    }
    return "Line";
}
