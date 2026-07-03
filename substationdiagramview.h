#ifndef SUBSTATIONDIAGRAMVIEW_H
#define SUBSTATIONDIAGRAMVIEW_H

#include <QGraphicsView>
#include <QHash>
#include <QList>
#include <QColor>
#include <QPointF>
#include <QMap>
#include <QSizeF>
#include <QRectF>
#include <QString>

#include "diagramnodeitem.h"
#include "substationlayout.h"

class QGraphicsScene;
class DiagramLinkItem;
class QPainter;
class QResizeEvent;
class QShowEvent;

class SubstationDiagramView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit SubstationDiagramView(QWidget *parent = nullptr);

    void setLayout(const SubstationLayout::Layout &layout);
    void fitToContent();
    void selectEquipment(const QString &equipmentKey);

signals:
    void equipmentActivated(const QString &equipmentKey);

private:
    struct ConnectionRecord
    {
        QString sourceKey;
        QString targetKey;
        DiagramLinkItem *item = nullptr;
    };

    void buildDiagram();
    DiagramNodeItem *addNode(const SubstationLayout::NodeSpec &nodeSpec);
    void addConnection(const SubstationLayout::ConnectionSpec &connectionSpec,
                       const QPointF &start,
                       const QPointF &end);
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void resizeEvent(QResizeEvent *event) override;
    QPointF anchorPoint(const DiagramNodeItem *node, bool rightSide) const;

    QGraphicsScene *m_scene;
    QHash<QString, DiagramNodeItem *> m_nodes;
    QList<ConnectionRecord> m_connections;
    SubstationLayout::Layout m_layout;
    bool m_fitPending;
};

#endif // SUBSTATIONDIAGRAMVIEW_H
