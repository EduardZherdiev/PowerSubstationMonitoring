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
    const SubstationLayout::Layout &substationLayout() const;
    SubstationLayout::Layout layoutWithCurrentPositions() const;
    void fitToContent();
    void selectEquipment(const QString &equipmentKey);
    void refreshTheme();

signals:
    void equipmentActivated(const QString &equipmentKey);

private:
    struct ConnectionRecord
    {
        QString sourceKey;
        QString targetKey;
        DiagramNodeItem *sourceNode = nullptr;
        DiagramNodeItem *targetNode = nullptr;
        DiagramLinkItem *item = nullptr;
    };

    void buildDiagram();
    DiagramNodeItem *addNode(const SubstationLayout::NodeSpec &nodeSpec);
    void addConnection(const SubstationLayout::ConnectionSpec &connectionSpec,
                       DiagramNodeItem *sourceNode,
                       DiagramNodeItem *targetNode);
    void updateConnectionLines();
    void updateNodePosition(const QString &equipmentKey, const QPointF &position);
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void resizeEvent(QResizeEvent *event) override;
    QPointF anchorPoint(const DiagramNodeItem *node, const DiagramNodeItem *other) const;

    QGraphicsScene *m_scene;
    QHash<QString, DiagramNodeItem *> m_nodes;
    QList<ConnectionRecord> m_connections;
    SubstationLayout::Layout m_layout;
    bool m_fitPending;
};

#endif // SUBSTATIONDIAGRAMVIEW_H
