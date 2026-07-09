#ifndef MONITORINGCHART_H
#define MONITORINGCHART_H

#include "telemetryhistory.h"

#include <QColor>
#include <QGraphicsScene>
#include <QString>

class QGraphicsSceneMouseEvent;
class QObject;

namespace MonitoringChart {

class Scene : public QGraphicsScene
{
public:
    using QGraphicsScene::QGraphicsScene;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
};

Scene *createScene(QObject *parent = nullptr);

void render(QGraphicsScene *scene,
            const QVector<TelemetrySample> &history,
            const QString &title,
            const QString &unit,
            const QColor &lineColor,
            const QColor &textColor,
            int windowSeconds);

} // namespace MonitoringChart

#endif // MONITORINGCHART_H
