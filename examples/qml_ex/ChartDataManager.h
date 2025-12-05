#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QColor>
#include <QVariant>
#include <vector>
#include <random>

// Indicator types that can be added to the chart
enum class IndicatorType {
    Line,
    Bars,
    Scatter,
    Shaded,
    Candles
};

// Represents a single data series on the chart
struct ChartSeries {
    QString name;
    IndicatorType type;
    QColor color;
    std::vector<float> xData;
    std::vector<float> yData;
    // For candlestick data
    std::vector<float> open;
    std::vector<float> high;
    std::vector<float> low;
    std::vector<float> close;
    bool visible = true;
};

class ChartDataManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int seriesCount READ seriesCount NOTIFY seriesChanged)
    Q_PROPERTY(QVariantList seriesInfo READ seriesInfo NOTIFY seriesChanged)

public:
    explicit ChartDataManager(QObject *parent = nullptr);
    ~ChartDataManager() override = default;

    // QML accessible methods
    Q_INVOKABLE void addIndicator(const QString &typeName, float dropX = 0, float dropY = 0);
    Q_INVOKABLE void removeIndicator(int index);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void toggleVisibility(int index);

    // C++ methods for ImGui rendering
    int seriesCount() const { return static_cast<int>(m_series.size()); }
    const std::vector<ChartSeries>& getAllSeries() const { return m_series; }
    QVariantList seriesInfo() const;

    // Generate sample data for demonstration
    void generateSampleData(ChartSeries& series, int pointCount = 100);
    void generateCandleData(ChartSeries& series, int pointCount = 50);

signals:
    void seriesChanged();
    void indicatorAdded(const QString& name, const QString& type);
    void indicatorRemoved(int index);

private:
    std::vector<ChartSeries> m_series;
    int m_nextId = 1;
    std::mt19937 m_rng;

    QColor getNextColor();
    static IndicatorType typeFromString(const QString& typeName);
    static QString typeToString(IndicatorType type);
};
