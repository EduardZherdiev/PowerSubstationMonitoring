#include "monitoringchart.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QHash>
#include <QLineF>
#include <QVariant>

#include <algorithm>
#include <limits>

namespace {

struct ChartState
{
    QVector<QGraphicsEllipseItem *> points;
    QGraphicsTextItem *detailsItem = nullptr;
    QString defaultDetailsText;
    qint64 selectedTimestamp = -1;
    bool selectionHooked = false;
};

QHash<QGraphicsScene *, ChartState> &chartStates()
{
    static QHash<QGraphicsScene *, ChartState> states;
    return states;
}

QString formatAxisValue(double value, const QString &unit)
{
    return QCoreApplication::translate("MonitoringChart", "%1 %2")
        .arg(QString::number(value, 'f', 1), unit);
}

QString formatPointText(const TelemetrySample &sample, const QString &unit)
{
    return QCoreApplication::translate("MonitoringChart", "%1  |  %2")
        .arg(sample.timestamp.toLocalTime().toString(QStringLiteral("HH:mm:ss")),
             formatAxisValue(sample.value, unit));
}

void updatePointSelection(QGraphicsScene *scene)
{
    if (!scene) {
        return;
    }

    ChartState &state = chartStates()[scene];
    const QList<QGraphicsItem *> selectedItems = scene->selectedItems();
    QGraphicsEllipseItem *selectedPoint = nullptr;

    if (!selectedItems.isEmpty()) {
        selectedPoint = qgraphicsitem_cast<QGraphicsEllipseItem *>(selectedItems.first());
    }

    for (QGraphicsEllipseItem *point : state.points) {
        if (!point) {
            continue;
        }

        const bool isSelected = point == selectedPoint;
        const QColor baseColor = point->data(3).value<QColor>();
        const QColor outlineColor = point->data(4).value<QColor>();
        point->setBrush(isSelected ? baseColor.lighter(130) : Qt::transparent);
        point->setPen(QPen(isSelected ? outlineColor.lighter(130) : Qt::transparent, isSelected ? 2.2 : 0.0));
        point->setZValue(isSelected ? 3.0 : 2.0);
    }

    if (!state.detailsItem) {
        return;
    }

    if (selectedPoint) {
        const QDateTime timestamp = QDateTime::fromSecsSinceEpoch(selectedPoint->data(0).toLongLong());
        const TelemetrySample sample{timestamp, selectedPoint->data(1).toDouble()};
        const QString unit = selectedPoint->data(2).toString();
        state.selectedTimestamp = selectedPoint->data(0).toLongLong();
        state.detailsItem->setPlainText(QCoreApplication::translate("MonitoringChart", "Selected: %1")
                                            .arg(formatPointText(sample, unit)));
    } else {
        state.detailsItem->setPlainText(state.defaultDetailsText);
    }
}

QString timeLabelFormatForWindow(int windowSeconds)
{
    if (windowSeconds <= 60) {
        return QStringLiteral("HH:mm:ss");
    }
    if (windowSeconds <= 3600) {
        return QStringLiteral("HH:mm:ss");
    }
    if (windowSeconds <= 24 * 3600) {
        return QStringLiteral("HH:mm");
    }
    return QStringLiteral("dd/MM HH:mm");
}

QString timeText(const QDateTime &timestamp, int windowSeconds)
{
    return timestamp.toLocalTime().toString(timeLabelFormatForWindow(windowSeconds));
}

QSizeF sceneSize(QGraphicsScene *scene)
{
    if (!scene || scene->views().isEmpty()) {
        return QSizeF(420.0, 240.0);
    }

    const QSize viewportSize = scene->views().first()->viewport()->size();
    return QSizeF(qMax(320.0, viewportSize.width() * 0.9), qMax(240.0, viewportSize.height() * 0.8));
}

} // namespace

namespace MonitoringChart {

Scene *createScene(QObject *parent)
{
    auto *scene = new Scene(parent);
    QObject::connect(scene, &QObject::destroyed, [scene]() {
        chartStates().remove(scene);
    });
    return scene;
}

void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!event) {
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    ChartState &state = chartStates()[this];
    QGraphicsEllipseItem *closestPoint = nullptr;
    qreal closestDistance = std::numeric_limits<qreal>::max();

    for (QGraphicsEllipseItem *point : state.points) {
        if (!point) {
            continue;
        }

        const qreal distance = QLineF(event->scenePos(), point->sceneBoundingRect().center()).length();
        if (distance < closestDistance) {
            closestDistance = distance;
            closestPoint = point;
        }
    }

    if (closestPoint && closestDistance <= 28.0) {
        clearSelection();
        closestPoint->setSelected(true);
        updatePointSelection(this);
        event->accept();
        return;
    }

    QGraphicsScene::mousePressEvent(event);
}

void render(QGraphicsScene *scene,
            const QVector<TelemetrySample> &history,
            const QString &title,
            const QString &unit,
            const QColor &lineColor,
            const QColor &textColor,
            int windowSeconds)
{
    if (!scene) {
        return;
    }

    scene->clearSelection();
    const QSizeF size = sceneSize(scene);
    scene->clear();
    scene->setSceneRect(0, 0, size.width(), size.height());
    scene->setBackgroundBrush(Qt::transparent);

    ChartState &state = chartStates()[scene];
    state.points.clear();
    state.detailsItem = nullptr;
    state.defaultDetailsText.clear();

    QFont titleFont(QStringLiteral("Segoe UI"), 10, QFont::Bold);
    QFont labelFont(QStringLiteral("Segoe UI"), 7);
    QFont axisFont(QStringLiteral("Segoe UI"), 6);

    QGraphicsTextItem *titleItem = scene->addText(title, titleFont);
    titleItem->setDefaultTextColor(textColor);
    titleItem->setPos(10, 4);

    if (history.isEmpty()) {
        QGraphicsTextItem *emptyItem = scene->addText(QCoreApplication::translate("MonitoringChart", "No data"), labelFont);
        emptyItem->setDefaultTextColor(textColor);
        emptyItem->setPos(10, size.height() / 2.0 - 10.0);
        return;
    }

    const QDateTime endTime = history.last().timestamp;
    const QDateTime startTime = endTime.addSecs(-windowSeconds);

    QVector<TelemetrySample> visible;
    visible.reserve(history.size());
    for (const TelemetrySample &sample : history) {
        if (sample.timestamp >= startTime) {
            visible.append(sample);
        }
    }

    if (visible.isEmpty()) {
        QGraphicsTextItem *emptyItem = scene->addText(QCoreApplication::translate("MonitoringChart", "No data in period"), labelFont);
        emptyItem->setDefaultTextColor(textColor);
        emptyItem->setPos(10, size.height() / 2.0 - 10.0);
        return;
    }

    double minValue = visible.first().value;
    double maxValue = visible.first().value;
    for (const TelemetrySample &sample : visible) {
        minValue = qMin(minValue, sample.value);
        maxValue = qMax(maxValue, sample.value);
    }
    const double range = qMax(0.1, maxValue - minValue);

    const qreal leftMargin = qMax<qreal>(56.0, size.width() * 0.15);
    const qreal rightMargin = qMax<qreal>(20.0, size.width() * 0.08);
    const qreal topMargin = qMax<qreal>(32.0, size.height() * 0.12);
    const qreal bottomMargin = qMax<qreal>(72.0, size.height() * 0.28);
    const QRectF plotRect(leftMargin,
                          topMargin,
                          size.width() - leftMargin - rightMargin,
                          size.height() - topMargin - bottomMargin);

    scene->addRect(plotRect, QPen(textColor, 1), QBrush(Qt::NoBrush));

    const int axisSteps = 4;
    for (int i = 0; i <= axisSteps; ++i) {
        const double fraction = static_cast<double>(i) / axisSteps;
        const qreal y = plotRect.top() + plotRect.height() * fraction;
        const double value = maxValue - (maxValue - minValue) * fraction;
        scene->addLine(plotRect.left(), y, plotRect.right(), y, QPen(textColor.lighter(145), 0.45, Qt::SolidLine));

        QGraphicsTextItem *axisItem = scene->addText(formatAxisValue(value, unit), axisFont);
        axisItem->setDefaultTextColor(textColor);
        const QRectF bounds = axisItem->boundingRect();
        axisItem->setPos(qMax<qreal>(4.0, plotRect.left() - bounds.width() - 8.0), y - bounds.height() / 2.0);
    }

    const int tickCount = 5;
    for (int i = 0; i <= tickCount; ++i) {
        const double fraction = static_cast<double>(i) / tickCount;
        const qreal x = plotRect.left() + plotRect.width() * fraction;
        scene->addLine(x, plotRect.top(), x, plotRect.bottom(), QPen(textColor, 0.4, Qt::DotLine));

        const QDateTime tickTime = startTime.addSecs(static_cast<int>(windowSeconds * fraction));
        QGraphicsTextItem *tickItem = scene->addText(timeText(tickTime, windowSeconds), axisFont);
        tickItem->setDefaultTextColor(textColor);
        const QRectF bounds = tickItem->boundingRect();
        tickItem->setPos(x - bounds.width() / 2.0, plotRect.bottom() + 2.0);
    }

    QPainterPath path;
    const qint64 totalSeconds = qMax<qint64>(1, startTime.secsTo(endTime));
    bool firstPoint = true;
    const int maximumInteractivePoints = qMax(2, static_cast<int>(plotRect.width() / 4.0));
    const int interactiveStep = qMax(1, visible.size() / maximumInteractivePoints);
    for (int sampleIndex = 0; sampleIndex < visible.size(); ++sampleIndex) {
        const TelemetrySample &sample = visible.at(sampleIndex);
        const qint64 elapsed = qMax<qint64>(0, startTime.secsTo(sample.timestamp));
        const double normalizedX = static_cast<double>(elapsed) / static_cast<double>(totalSeconds);
        const double normalizedY = (sample.value - minValue) / range;
        const qreal x = plotRect.left() + normalizedX * plotRect.width();
        const qreal y = plotRect.bottom() - normalizedY * plotRect.height();
        if (firstPoint) {
            path.moveTo(x, y);
            firstPoint = false;
        } else {
            path.lineTo(x, y);
        }

        const bool isInteractivePoint = sampleIndex % interactiveStep == 0 || sampleIndex == visible.size() - 1;
        if (!isInteractivePoint) {
            continue;
        }

        QGraphicsEllipseItem *point = scene->addEllipse(x - 3.5, y - 3.5, 7.0, 7.0, QPen(Qt::NoPen), QBrush(Qt::NoBrush));
        point->setFlag(QGraphicsItem::ItemIsSelectable, true);
        point->setAcceptedMouseButtons(Qt::LeftButton);
        point->setData(0, sample.timestamp.toSecsSinceEpoch());
        point->setData(1, sample.value);
        point->setData(2, unit);
        point->setData(3, lineColor);
        point->setData(4, textColor);
        point->setToolTip(formatPointText(sample, unit));
        point->setZValue(2.0);
        state.points.append(point);
    }

    scene->addPath(path, QPen(lineColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    state.defaultDetailsText = QCoreApplication::translate("MonitoringChart", "Current: %1")
                                   .arg(formatPointText(visible.last(), unit));

    QGraphicsTextItem *statsItem = scene->addText(
        QCoreApplication::translate("MonitoringChart", "%1 %2  min %3  max %4")
            .arg(QString::number(visible.last().value, 'f', 1),
                 unit,
                 QString::number(minValue, 'f', 1),
                 QString::number(maxValue, 'f', 1)),
        labelFont);
    statsItem->setDefaultTextColor(textColor);
    statsItem->setPos(10, size.height() - 24.0);

    state.detailsItem = scene->addText(state.defaultDetailsText, labelFont);
    state.detailsItem->setDefaultTextColor(textColor);
    state.detailsItem->setPos(plotRect.left(), qMax<qreal>(0.0, plotRect.top() - 22.0));

    QGraphicsTextItem *windowItem = scene->addText(QStringLiteral("%1").arg(timeText(endTime, windowSeconds)), axisFont);
    windowItem->setDefaultTextColor(textColor);
    windowItem->setPos(size.width() - windowItem->boundingRect().width() - 12.0, 6.0);

    if (!state.selectionHooked) {
        state.selectionHooked = true;
        QObject::connect(scene, &QGraphicsScene::selectionChanged, scene, [scene]() {
            updatePointSelection(scene);
        });
    }

    if (state.selectedTimestamp >= 0) {
        for (QGraphicsEllipseItem *point : state.points) {
            if (point && point->data(0).toLongLong() == state.selectedTimestamp) {
                point->setSelected(true);
                break;
            }
        }
    }

    updatePointSelection(scene);
}

} // namespace MonitoringChart
